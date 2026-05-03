#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <time.h>
#include "protocol.h"

// menyimpan socket dan username client
int  client_sockets[MAX_CLIENTS];
char username[MAX_CLIENTS][50];

// mencatat log ke file history.log
void log_event(char *msg) {
    FILE *f = fopen("history.log", "a");
    if (!f) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    fprintf(f, "[%04d-%02d-%02d %02d:%02d:%02d] %s\n",
        t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
        t->tm_hour, t->tm_min, t->tm_sec, msg);
    fclose(f);
}

// mengirim pesan ke semua client kecuali pengirim
void broadcast(char *msg, int pengirim) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] != 0 && client_sockets[i] != pengirim) {
            send(client_sockets[i], msg, strlen(msg), 0);
        }
    }
}

// mengecek apakah username sudah terpakai
int username_exist(char *nama) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (strcmp(username[i], nama) == 0)
            return 1;
    }
    return 0;
}

// menghapus client dari daftar saat disconnect
void hapus_client(int sd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] == sd) {
            client_sockets[i] = 0;
            strcpy(username[i], "");
            break;
        }
    }
    close(sd);
}

// mendaftarkan socket client ke slot kosong
int daftar_client(int sd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] == 0) {
            client_sockets[i] = sd;
            return i;
        }
    }
    return -1; // penuh
}

int main() {
    int server_fd, new_socket, addrlen;
    struct sockaddr_in address;
    fd_set readfds;
    char buffer[BUFFER_SIZE];

    // inisialisasi semua slot client
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = 0;
        strcpy(username[i], "");
    }

    // membuat dan konfigurasi socket server
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("[ERROR] Gagal buat socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family      = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port        = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("[ERROR] Gagal bind");
        exit(EXIT_FAILURE);
    }

    listen(server_fd, 5);
    log_event("[System] SERVER ONLINE");
    printf("[Server] Berjalan di port %d...\n", PORT);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        int max_sd = server_fd;

        // masukkan semua client aktif ke set
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];
            if (sd > 0) FD_SET(sd, &readfds);
            if (sd > max_sd) max_sd = sd;
        }

        select(max_sd + 1, &readfds, NULL, NULL, NULL);

        // koneksi client baru masuk
        if (FD_ISSET(server_fd, &readfds)) {
            addrlen    = sizeof(address);
            new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);

            if (new_socket < 0) {
                perror("[ERROR] Gagal accept");
                continue;
            }

            int slot = daftar_client(new_socket);
            if (slot == -1) {
                char *penuh = "[Server] Maaf, server penuh.\n";
                send(new_socket, penuh, strlen(penuh), 0);
                close(new_socket);
                continue;
            }

            // minta username dari client baru
            char *minta_nama = "[Server] Masukkan username: ";
            send(new_socket, minta_nama, strlen(minta_nama), 0);

            memset(buffer, 0, BUFFER_SIZE);
            int panjang = read(new_socket, buffer, BUFFER_SIZE);
            if (panjang <= 0) {
                hapus_client(new_socket);
                continue;
            }

            buffer[strcspn(buffer, "\r\n")] = 0;

            if (username_exist(buffer)) {
                char *duplikat = "[Server] Username sudah dipakai.\n";
                send(new_socket, duplikat, strlen(duplikat), 0);
                hapus_client(new_socket);
                continue;
            }

            strncpy(username[slot], buffer, 49);

            char notif[BUFFER_SIZE];
            snprintf(notif, BUFFER_SIZE, "[Server] %s bergabung ke chat.", username[slot]);
            log_event(notif);
            broadcast(notif, new_socket);

            char sambutan[BUFFER_SIZE];
            snprintf(sambutan, BUFFER_SIZE, "[Server] Selamat datang, %s!\n", username[slot]);
            send(new_socket, sambutan, strlen(sambutan), 0);

            printf("[INFO] Client baru: %s (fd=%d)\n", username[slot], new_socket);
        }

        // cek pesan dari client yang sudah terhubung
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];
            if (sd == 0 || !FD_ISSET(sd, &readfds)) continue;

            memset(buffer, 0, BUFFER_SIZE);
            int panjang = read(sd, buffer, BUFFER_SIZE);

            if (panjang <= 0) {
                // client disconnect
                char notif[BUFFER_SIZE];
                snprintf(notif, BUFFER_SIZE, "[Server] %s keluar.", username[i]);
                log_event(notif);
                broadcast(notif, sd);
                printf("[INFO] %s disconnect.\n", username[i]);
                hapus_client(sd);
                continue;
            }

            buffer[strcspn(buffer, "\r\n")] = 0;

            // perintah /exit dari client
            if (strncmp(buffer, "/exit", 5) == 0) {
                char notif[BUFFER_SIZE];
                snprintf(notif, BUFFER_SIZE, "[Server] %s telah keluar.", username[i]);
                log_event(notif);
                broadcast(notif, sd);
                hapus_client(sd);
                continue;
            }

            // perintah admin: /kick <username>
            if (strncmp(buffer, "/kick ", 6) == 0) {
                char *target = buffer + 6;
                int ketemu   = 0;
                for (int j = 0; j < MAX_CLIENTS; j++) {
                    if (strcmp(username[j], target) == 0) {
                        char pesan_kick[BUFFER_SIZE];
                        snprintf(pesan_kick, BUFFER_SIZE, "[Server] Kamu di-kick oleh admin.\n");
                        send(client_sockets[j], pesan_kick, strlen(pesan_kick), 0);
                        hapus_client(client_sockets[j]);
                        ketemu = 1;
                        break;
                    }
                }
                if (!ketemu) {
                    char *gagal = "[Server] Username tidak ditemukan.\n";
                    send(sd, gagal, strlen(gagal), 0);
                }
                continue;
            }

            // broadcast pesan normal ke semua client
            char pesan[BUFFER_SIZE + 60];
            snprintf(pesan, sizeof(pesan), "[%s]: %s\n", username[i], buffer);
            log_event(pesan);
            broadcast(pesan, sd);
        }
    }

    close(server_fd);
    return 0;
}
