# SISOP-3-2026-IT-106
# PRAKTIKUM MODUL 3 SISTEM OPERASI

## SOAL_1

### Penjelasan

Proyek ini mengimplementasikan sistem chat jaringan bernama **The Wired** yang terdiri dari dua komponen utama: `server.c` sebagai server pusat dan `navi.c` sebagai unit klien (NAVI). Sistem ini menggunakan konsep **Thread**, **Socket (IPC)**, **select()**, dan **Mutex** dari Modul 3 Sistem Operasi.

---

### server.c

File server yang menangani banyak klien secara konkuren menggunakan `select()` untuk memantau semua socket sekaligus dalam satu thread. Server mendukung fitur broadcast, username unik, perintah admin `/kick`, dan logging ke `history.log`.

#### Kode

```c
// memantau semua socket (server + semua client) sekaligus dengan select()
FD_ZERO(&readfds);
FD_SET(server_fd, &readfds);

for (int i = 0; i < MAX_CLIENTS; i++) {
    int sd = client_sockets[i];
    if (sd > 0) FD_SET(sd, &readfds);
    if (sd > max_sd) max_sd = sd;
}

select(max_sd + 1, &readfds, NULL, NULL, NULL);
```

Broadcast pesan ke semua client kecuali pengirim:

```c
void broadcast(char *msg, int pengirim) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] != 0 && client_sockets[i] != pengirim) {
            send(client_sockets[i], msg, strlen(msg), 0);
        }
    }
}
```

### navi.c

File klien yang terhubung ke server. Menggunakan **pthread** untuk menjalankan fungsi kirim dan terima secara asinkron dalam satu proses — tanpa fork.

#### Kode

```c
// jalankan thread penerima pesan di background
pthread_create(&thread_terima, NULL, terima_pesan, NULL);

// loop utama: baca input user lalu kirim ke server
while (berjalan) {
    fgets(pesan, BUFFER_SIZE, stdin);
    send(sock, pesan, strlen(pesan), 0);

    if (strncmp(pesan, "/exit", 5) == 0) {
        berjalan = 0;
        break;
    }
}
```

Thread penerima berjalan di background:

```c
void *terima_pesan(void *arg) {
    char buffer[BUFFER_SIZE];
    while (berjalan) {
        int panjang = read(sock, buffer, BUFFER_SIZE);
        if (panjang <= 0) { berjalan = 0; break; }
        printf("%s", buffer);
        fflush(stdout);
    }
    return NULL;
}
```


### Cara Kompilasi

```bash
gcc -Wall -o server server.c
gcc -Wall -pthread -o navi navi.c
```

> **Penting:** Flag `-pthread` wajib disertakan pada kompilasi `navi.c` karena menggunakan POSIX Thread.

---

### Cara Menjalankan

```bash
# Terminal 1 — jalankan server DULU
./server

# Terminal 2 — klien pertama
./navi

# Terminal 3 — klien kedua
./navi
```

---

### Output

```
# Server saat pertama kali dijalankan
[Server] Berjalan di port 12345...

# Client berhasil konek
[Client] Menghubungkan ke server 127.0.0.1:12345...
[Client] Berhasil terhubung!
[Server] Masukkan username: alice
[Server] Selamat datang, alice!

# Chat antar user
> hello lain
[lain]: hello alice

# Percobaan username duplikat
[Server] Masukkan username: alice
[Server] Username sudah dipakai.

# Perintah admin kick
> /kick lain
# (lain menerima) [Server] Kamu di-kick oleh admin.

# Keluar dari chat
> /exit
[Client] Keluar dari chat.
[Client] Koneksi ditutup.
```

---

### history.log

Setiap aktivitas tercatat otomatis ke `history.log` dengan format `[YYYY-MM-DD HH:MM:SS] pesan`.

```
[2026-05-03 19:06:40] [System] SERVER ONLINE
[2026-05-03 19:06:46] [Server] alice bergabung ke chat.
[2026-05-03 19:06:50] [Server] lain bergabung ke chat.
[2026-05-03 19:06:56] [alice]: hello lain
[2026-05-03 19:06:59] [lain]: hello alice
[2026-05-03 19:07:11] [Server] lain keluar.
[2026-05-03 19:07:27] [Server] alice telah keluar.
```
