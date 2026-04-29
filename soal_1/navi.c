#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <pthread.h>

#define PORT        12345
#define BUFFER_SIZE 1024
#define ALAMAT_SERVER "127.0.0.1"

int sock;
volatile int berjalan = 1;

// thread khusus untuk menerima pesan dari server
void *terima_pesan(void *arg) {
    char buffer[BUFFER_SIZE];

    while (berjalan) {
        memset(buffer, 0, BUFFER_SIZE);
        int panjang = read(sock, buffer, BUFFER_SIZE);

        if (panjang <= 0) {
            printf("\n[Client] Koneksi ke server terputus.\n");
            berjalan = 0;
            break;
        }

        printf("%s", buffer);
        fflush(stdout);
    }

    return NULL;
}

int main() {
    struct sockaddr_in server;
    char pesan[BUFFER_SIZE];
    pthread_t thread_terima;

    // membuat socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("[ERROR] Gagal buat socket");
        exit(EXIT_FAILURE);
    }

    // konfigurasi alamat server
    server.sin_family = AF_INET;
    server.sin_port   = htons(PORT);
    inet_pton(AF_INET, ALAMAT_SERVER, &server.sin_addr);

    // mencoba konek ke server
    printf("[Client] Menghubungkan ke server %s:%d...\n", ALAMAT_SERVER, PORT);
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("[ERROR] Gagal konek ke server");
        exit(EXIT_FAILURE);
    }
    printf("[Client] Berhasil terhubung!\n");

    // jalankan thread penerima pesan di background
    pthread_create(&thread_terima, NULL, terima_pesan, NULL);

    // loop utama: baca input user lalu kirim ke server
    while (berjalan) {
        memset(pesan, 0, BUFFER_SIZE);

        if (fgets(pesan, BUFFER_SIZE, stdin) == NULL) break;

        // bersihkan newline sebelum kirim
        pesan[strcspn(pesan, "\r\n")] = '\n';

        send(sock, pesan, strlen(pesan), 0);

        // keluar jika user ketik /exit
        if (strncmp(pesan, "/exit", 5) == 0) {
            printf("[Client] Keluar dari chat.\n");
            berjalan = 0;
            break;
        }
    }

    pthread_cancel(thread_terima);
    pthread_join(thread_terima, NULL);
    close(sock);

    printf("[Client] Koneksi ditutup.\n");
    return 0;
}