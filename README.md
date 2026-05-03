# SISOP-3-2026-IT-106
# PRAKTIKUM MODUL 3 SISTEM OPERASI

## SOAL_1

### Penjelasan

Proyek ini mengimplementasikan sistem chat jaringan bernama **The Wired** yang terdiri dari tiga file utama: `wired.c` sebagai server pusat, `navi.c` sebagai unit klien, dan `protocol.h` sebagai header bersama yang menyimpan konstanta konfigurasi. Sistem ini menggunakan konsep **Thread**, **Socket (IPC)**, dan **select()** dari Modul 3 Sistem Operasi.

---

### protocol.h

Header bersama yang digunakan oleh `wired.c` dan `navi.c`. Berisi konstanta konfigurasi agar tidak hardcode di masing-masing file.

#### Kode

```c
#ifndef PROTOCOL_H
#define PROTOCOL_H

#define PORT          12345
#define MAX_CLIENTS   10
#define BUFFER_SIZE   1024
#define ADMIN_PASS    "protocol7"
#define ALAMAT_SERVER "127.0.0.1"

#endif
```

---

### wired.c

File server yang menangani banyak klien secara konkuren menggunakan `select()` untuk memantau semua socket sekaligus dalam satu thread. Mendukung broadcast, username unik, dan perintah `/kick`.

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
# Menggunakan Makefile
make

# Atau manual
gcc -Wall -o wired wired.c
gcc -Wall -pthread -o navi navi.c
```

> **Penting:** Flag `-pthread` wajib disertakan pada kompilasi `navi.c` karena menggunakan POSIX Thread.

---

### Cara Menjalankan

```bash
# Terminal 1 — jalankan server DULU
./wired

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

## NOTE :
Saya melakukan commit terbaru dengan menambahkan wired karena kesalahan saya, saya menggabung sistem wired menjadi satu di dalam protocol, dan melakukan commit terbaru untuk memisahkan 2 program tersebut
