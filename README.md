# Sisop-4-2025-IT20

## Modul 4 Sistem Operasi 2025
- **Mey Rosalina NRP 5027241004**
- **Rizqi Akbar Sukirman Putra NRP 5027241044**
- **M. Alfaeran Auriga Ruswandi NRP 5027241115**

### soal 1
*Tujuan*
- Mendownload file .zip dari Google Drive.
- Unzip file tersebut yang berisi file teks hexadecimal (contoh: 1.txt, 2.txt, dst).
- Mengonversi string hexadecimal di setiap file menjadi file gambar .png.
- Menyimpan hasil konversi di direktori image/.
- Mencatat aktivitas konversi ke dalam conversion.log.

Struktur Folder
Setelah unzip:
```
.
├── anomali/
│   ├── 1.txt
│   ├── ...
│   ├── image/
│   │   ├── 1_image_YYYY-mm-dd_HH:MM:SS.png
│   │   └── ...
│   └── conversion.log
```
```
[YYYY-mm-dd][HH:MM:SS]: Successfully converted hexadecimal text 1.txt to 1_image_YYYY-mm-dd_HH:MM:SS.png
```
Masalah yang Ditemukan :

- Beberapa gambar hasil konversi rusak atau tidak terbaca.
- Program tidak fleksibel untuk eksplorasi hasil karena hasilnya langsung ditulis ke disk.

**II. Versi Revisi (Berbasis FUSE)**

*Tujuan*
- Memanfaatkan filesystem virtual FUSE untuk menampilkan hasil konversi secara on-demand.
- Ketika pengguna mengakses file .png dalam folder mount (mnt/), sistem akan:
- Membaca isi .txt hexadecimal dari anomali/.
- Mengonversinya ke gambar PNG saat itu juga.
- Menyediakan hasilnya langsung seolah-olah file .png memang sudah ada.
- Menyediakan conversion.log secara virtual.

Struktur Folder Setelah Mount:
```
mnt/
├── 1_image_YYYY-mm-dd_HH:MM:SS.png  (dibuat saat dibuka)
├── ...
└── conversion.log  (berisi daftar konversi)
```

*Keunggulan :*

- File tidak benar-benar ditulis ke disk, hanya tampil saat dibuka.
- Fleksibel, ringan, dan cocok untuk sistem berbasis observasi anomali seperti proyek ini.

*Masalah dan Perbaikan :*

- Bug awal terkait buffer snprintf diperbaiki.
- Error errno karena lupa #include <errno.h> juga sudah diselesaikan.
- Mounting gagal karena mnt/ dimiliki root: disarankan gunakan mkdir mnt && chmod u+w mnt.

### soal 2
```
#define FUSE_USE_VERSION 31
#include <fuse3/fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
```


```
#define PART_SIZE 1024
#define PART_COUNT 14
#define MAX_FILE_SIZE (1024 * 1024 * 100)
#define FILENAME "Baymax.jpeg"
```

- PART_SIZE: ukuran setiap pecahan file = 1KB

- PART_COUNT: jumlah pecahan default file ```Baymax.jpeg``` = 14

- MAX_FILE_SIZE: batas ukuran file maksimum: 100 MB

- FILENAME: nama file utama virtual yang ditampilkan


```
char base_dir[1024];
char activity_log_path[1024];
```

Menyimpan direktori kerja saat ini dan path untuk ```activity.log```


```
static char *write_buffer = NULL;
static size_t write_size = 0;
static char last_read_file[256] = "";
static time_t last_read_time = 0;
```


```
void get_fragment_path(char *dest, const char *filename, int index) {
snprintf(dest, 1024, "%s/relics/%s.%03d", base_dir, filename, index);
}
```

Membentuk path ke file pecahan: ```relics/nama_file.000```, dll.


```
void log_activity(const char *action, const char *filename, const char *extra) {
FILE *log = fopen(activity_log_path, "a");
if (!log) return;

time_t now = time(NULL);
struct tm *t = localtime(&now);

fprintf(log, "[%04d-%02d-%02d %02d:%02d:%02d] %s: %s%s\n",
t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
t->tm_hour, t->tm_min, t->tm_sec,
action, filename, extra ? extra : "");

fclose(log);
}
```

Mencatat semua aktivitas (READ, WRITE, DELETE, COPY) ke file ```activity.log```


```
static int baymax_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
off_t offset, struct fuse_file_info *fi,
enum fuse_readdir_flags flags) {
if (strcmp(path, "/") != 0) return -ENOENT;
filler(buf, ".", NULL, 0, 0);
filler(buf, "..", NULL, 0, 0);
filler(buf, FILENAME, NULL, 0, 0);
return 0;
}
```

Saat folder ```mount_dir``` dibuka, hanya menampilkan:

```.``` , ``` ..``` , dan file ```Baymax.jpeg```


```
static int baymax_getattr(const char *path, struct stat *st, struct fuse_file_info *fi) {
(void) fi;
memset(st, 0, sizeof(struct stat));

if (strcmp(path, "/") == 0) {
st->st_mode = S_IFDIR | 0755;
st->st_nlink = 2;
return 0;
}

if (strcmp(path + 1, FILENAME) == 0) {
st->st_mode = S_IFREG | 0644;
st->st_nlink = 1;
off_t total_size = 0;
for (int i = 0; i < PART_COUNT; i++) {
char part_path[1024];
get_fragment_path(part_path, FILENAME, i);
FILE *fp = fopen(part_path, "rb");
if (!fp) break;
fseek(fp, 0, SEEK_END);
total_size += ftell(fp);
fclose(fp);
}
st->st_size = total_size;
return 0;
}

return -ENOENT;
}
```

Mengembalikan metadata:

- Jika ```/``` → direktori

- Jika ```/Baymax.jpeg``` → file hasil gabungan dari semua .000–.013


```
static int baymax_open(const char *path, struct fuse_file_info *fi) {
if (strcmp(path + 1, FILENAME) != 0) return -ENOENT;
return 0;
}
```
Mengizinkan membuka hanya file ```Baymax.jpeg```, lainnya ```ENOENT```


```
static int baymax_read(const char *path, char *buf, size_t size, off_t offset,
struct fuse_file_info *fi) {
if (strcmp(path + 1, FILENAME) != 0) return -ENOENT;

time_t now = time(NULL);
log_activity("READ", FILENAME, NULL);
if (strcmp(last_read_file, FILENAME) == 0 && now - last_read_time <= 2) {
log_activity("COPY", FILENAME, " -> (possible copy detected)");
}
strcpy(last_read_file, FILENAME);
last_read_time = now;

size_t read_total = 0;
off_t current_offset = 0;

for (int i = 0; i < PART_COUNT && read_total < size; i++) {
char part_path[1024];
get_fragment_path(part_path, FILENAME, i);
FILE *fp = fopen(part_path, "rb");
if (!fp) break;
fseek(fp, 0, SEEK_END);
size_t part_size = ftell(fp);
fseek(fp, 0, SEEK_SET);

if (offset < current_offset + part_size) {
size_t local_offset = offset > current_offset ? offset - current_offset : 0;fseek(fp, local_offset, SEEK_SET);
size_t to_read = part_size - local_offset;
if (to_read > size - read_total) to_read = size - read_total;
fread(buf + read_total, 1, to_read, fp);
read_total += to_read;
}

current_offset += part_size;
fclose(fp);
}

return read_total;
}
```

- Membaca isi ```Baymax.jpeg``` dengan menggabungkan seluruh pecahan ```.000``` hingga ```.013```

- Mencatat aktivitas ```READ``` dan ```COPY``` ke log jika dua kali dalam 2 detik


```
static int baymax_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
if (write_buffer) free(write_buffer);
write_buffer = malloc(MAX_FILE_SIZE);
if (!write_buffer) return -ENOMEM;
write_size = 0;
return 0;
}
```
Menyiapkan buffer untuk menulis file baru sebelum dipecah


```
static int baymax_write(const char *path, const char *buf, size_t size, off_t offset,
struct fuse_file_info *fi) {
if (!write_buffer) return -EIO;
if (offset + size > MAX_FILE_SIZE) return -EFBIG;
memcpy(write_buffer + offset, buf, size);
if (offset + size > write_size) write_size = offset + size;
return size;
}
```

Menyimpan data sementara ke write_buffer, hingga nanti diflush (ditulis ke disk)


```
static int write_fragments(const char *filename) {
int parts = (write_size + PART_SIZE - 1) / PART_SIZE;
for (int i = 0; i < parts; i++) {
char part_path[1024];
get_fragment_path(part_path, filename, i);
FILE *fp = fopen(part_path, "wb");
if (!fp) return -EIO;
size_t chunk_size = (i == parts - 1) ? (write_size % PART_SIZE) : PART_SIZE;
if (chunk_size == 0) chunk_size = PART_SIZE;
fwrite(write_buffer + i * PART_SIZE, 1, chunk_size, fp);
fclose(fp);
}
return parts;
}
```

Memecah isi buffer ke dalam file .000, .001, dst. disimpan di folder ```relics/```


```
static int baymax_flush(const char *path, struct fuse_file_info *fi) {
const char *filename = path + 1;
int parts = write_fragments(filename);
if (parts < 0) return parts;

char extra[1024] = " -> ";
for (int i = 0; i < parts; i++) {
char part_name[64];
snprintf(part_name, sizeof(part_name), "%s.%03d", filename, i);
strcat(extra, part_name);
if (i < parts - 1) strcat(extra, ", ");
}

log_activity("WRITE", filename, extra);
free(write_buffer);
write_buffer = NULL;
write_size = 0;
return 0;
}
```

Setelah file selesai ditulis lalu memecah file, tulis ke ```relics/```, dan catat di log


```
static int baymax_unlink(const char *path) {
const char *filename = path + 1;
char deleted_parts[1024] = "";
int deleted_count = 0;

for (int i = 0; i < 1000; i++) {
char part_path[1024];
get_fragment_path(part_path, filename, i);
if (access(part_path, F_OK) == 0) {
remove(part_path);
char tmp[64];
snprintf(tmp, sizeof(tmp), "%s.%03d", filename, i);
if (deleted_count > 0) strcat(deleted_parts, " - ");
strcat(deleted_parts, tmp);
deleted_count++;
} else {
break;
}
}

if (deleted_count > 0) {
log_activity("DELETE", deleted_parts, "");
}

return 0;
}
```

jika file dihapus dari ```mount_dir```, maka semua pecahannya juga dihapus dari ```relics/```

Semua penghapusan dicatat di ```activity.log```


```
static const struct fuse_operations baymax_oper = {
.getattr = baymax_getattr,
.readdir = baymax_readdir,
.open = baymax_open,
.read = baymax_read,
.create = baymax_create,
.write = baymax_write,
.flush = baymax_flush,
.unlink = baymax_unlink,
};
```


```
int main(int argc, char *argv[]) {
getcwd(base_dir, sizeof(base_dir));
snprintf(activity_log_path, sizeof(activity_log_path), "%s/activity.log", base_dir);
umask(0);
return fuse_main(argc, argv, &baymax_oper, NULL);
}
```

Mengatur base_dir dan path log

Menjalankan fuse_main() untuk memulai sistem file virtual
### soal 3
### soal 4

### maimai_fs.c
```c
#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/time.h>
#include <limits.h>
#include <linux/limits.h>
#include <ctype.h>
#include <zlib.h>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#ifndef S_IFDIR
#define S_IFDIR 0040000 
#endif
#ifndef S_IFREG
#define S_IFREG 0100000 
#endif
#ifndef DT_DIR
#define DT_DIR 4 
#endif
#ifndef DT_REG
#define DT_REG 8
#endif
static const char *maimai_root_dir = "/tmp/maimai_data";
static char chiho_dir[PATH_MAX];
static char fuse_dir[PATH_MAX];
#define AES_BLOCK_SIZE 16
#define AES_KEY_SIZE 32
#define IV_SIZE 16
static const unsigned char heaven_key[AES_KEY_SIZE] = "maimai_heaven_chiho_aes_key_256bit";

// === AREA STARTER FUNCTIONS ===

static int is_starter_path(const char *path) {
    return (strncmp(path, "/fuse_dir/starter/", 18) == 0);
}

static void transform_starter_path(char *result, const char *path) {
    char filename[PATH_MAX];
    strcpy(filename, path + 18);
    sprintf(result, "%s/chiho/starter/%s.mai", maimai_root_dir, filename);
}

// === AREA METRO FUNCTIONS ===

static int is_metro_path(const char *path) {
    return (strncmp(path, "/fuse_dir/metro/", 16) == 0);
}

static void transform_metro_path(char *result, const char *path) {
    char filename[PATH_MAX];
    strcpy(filename, path + 16);
    sprintf(result, "%s/chiho/metro/%s.ccc", maimai_root_dir, filename);
}

static void metro_encrypt(const char *input, char *output, size_t size) {
    for (size_t i = 0; i < size; i++) {
        // Shift setiap karakter berdasarkan posisinya (i % 256)
        output[i] = (input[i] + (i % 256)) % 256;
    }
}

static void metro_decrypt(const char *input, char *output, size_t size) {
    for (size_t i = 0; i < size; i++) {
        output[i] = (input[i] - (i % 256) + 256) % 256;
    }
}

// === AREA DRAGON FUNCTIONS ===

static int is_dragon_path(const char *path) {
    return (strncmp(path, "/fuse_dir/dragon/", 17) == 0);
}

static void transform_dragon_path(char *result, const char *path) {
    char filename[PATH_MAX];
    strcpy(filename, path + 17);
    sprintf(result, "%s/chiho/dragon/%s.rot", maimai_root_dir, filename);
}

static void rot13(const char *input, char *output, size_t size) {
    for (size_t i = 0; i < size; i++) {
        char c = input[i];
        if (isalpha(c)) {
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
                if ((c >= 'a' && c <= 'm') || (c >= 'A' && c <= 'M'))
                    c += 13;
                else
                    c -= 13;
            }
        }
        output[i] = c;
    }
}

// === AREA BLACKROSE FUNCTIONS ===

static int is_blackrose_path(const char *path) {
    return (strncmp(path, "/fuse_dir/blackrose/", 20) == 0);
}

static void transform_blackrose_path(char *result, const char *path) {
    char filename[PATH_MAX];
    strcpy(filename, path + 20);
    sprintf(result, "%s/chiho/blackrose/%s.bin", maimai_root_dir, filename);
}

// === AREA HEAVEN FUNCTIONS ===

static int is_heaven_path(const char *path) {
    return (strncmp(path, "/fuse_dir/heaven/", 17) == 0);
}

static void transform_heaven_path(char *result, const char *path) {
    char filename[PATH_MAX];
    strcpy(filename, path + 17);
    sprintf(result, "%s/chiho/heaven/%s.enc", maimai_root_dir, filename);
}
static void handle_openssl_error() {
    ERR_print_errors_fp(stderr);
}

static int heaven_encrypt(const unsigned char *plaintext, int plaintext_len, 
                         unsigned char *ciphertext, int *ciphertext_len) {
    EVP_CIPHER_CTX *ctx;
    int len;
    int ciphertext_length;
    unsigned char iv[AES_BLOCK_SIZE];
    
    // Generate random IV
    if (!RAND_bytes(iv, AES_BLOCK_SIZE)) {
        handle_openssl_error();
        return 0;
    }
    
    // Copy IV to the beginning of ciphertext
    memcpy(ciphertext, iv, AES_BLOCK_SIZE);
    ciphertext_length = AES_BLOCK_SIZE;
    
    // Create and initialize the context
    if (!(ctx = EVP_CIPHER_CTX_new())) {
        handle_openssl_error();
        return 0;
    }
    
    // Initialize the encryption operation
    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, heaven_key, iv)) {
        handle_openssl_error();
        EVP_CIPHER_CTX_free(ctx);
        return 0;
    }
    
    // Encrypt the plaintext
    if (1 != EVP_EncryptUpdate(ctx, ciphertext + ciphertext_length, &len, 
                               plaintext, plaintext_len)) {
        handle_openssl_error();
        EVP_CIPHER_CTX_free(ctx);
        return 0;
    }
    ciphertext_length += len;
    
    // Finalize the encryption
    if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + ciphertext_length, &len)) {
        handle_openssl_error();
        EVP_CIPHER_CTX_free(ctx);
        return 0;
    }
    ciphertext_length += len;
    
    EVP_CIPHER_CTX_free(ctx);
    
    *ciphertext_len = ciphertext_length;
    return 1;
}

static int heaven_decrypt(const unsigned char *ciphertext, int ciphertext_len, 
                         unsigned char *plaintext, int *plaintext_len) {
    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_length;
    unsigned char iv[AES_BLOCK_SIZE];
    
    // Check if we have at least IV size bytes
    if (ciphertext_len <= AES_BLOCK_SIZE) {
        return 0;
    }
    
    // Extract the IV from the beginning of ciphertext
    memcpy(iv, ciphertext, AES_BLOCK_SIZE);
    
    // Create and initialize the context
    if (!(ctx = EVP_CIPHER_CTX_new())) {
        handle_openssl_error();
        return 0;
    }
    
    // Initialize the decryption operation
    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, heaven_key, iv)) {
        handle_openssl_error();
        EVP_CIPHER_CTX_free(ctx);
        return 0;
    }
    
    // Decrypt the ciphertext
    if (1 != EVP_DecryptUpdate(ctx, plaintext, &len, 
                              ciphertext + AES_BLOCK_SIZE, ciphertext_len - AES_BLOCK_SIZE)) {
        handle_openssl_error();
        EVP_CIPHER_CTX_free(ctx);
        return 0;
    }
    plaintext_length = len;
    
    // Finalize the decryption
    if (1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) {
        handle_openssl_error();
        EVP_CIPHER_CTX_free(ctx);
        return 0;
    }
    plaintext_length += len;
    
    EVP_CIPHER_CTX_free(ctx);
    
    *plaintext_len = plaintext_length;
    return 1;
}

// === AREA SKYSTREET FUNCTIONS ===

static int is_skystreet_path(const char *path) {
    return (strncmp(path, "/fuse_dir/skystreet/", 20) == 0);
}

static void transform_skystreet_path(char *result, const char *path) {
    char filename[PATH_MAX];
    strcpy(filename, path + 20);
    sprintf(result, "%s/chiho/skystreet/%s.gz", maimai_root_dir, filename);
}

static int skystreet_compress(const unsigned char *data, size_t data_len, 
                             unsigned char **compressed_data, size_t *compressed_len) {
    z_stream strm;
    int ret;
    size_t buf_size = data_len + (data_len / 100) + 13;
    *compressed_data = (unsigned char*)malloc(buf_size);
    if (!*compressed_data) 
        return Z_MEM_ERROR;

    // Initialize compression stream
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 
                      31,
                      8, Z_DEFAULT_STRATEGY);
    if (ret != Z_OK) {
        free(*compressed_data);
        return ret;
    }

    // Set input data
    strm.avail_in = data_len;
    strm.next_in = (unsigned char*)data;
    
    // Set output buffer
    strm.avail_out = buf_size;
    strm.next_out = *compressed_data;
    
    // Compress
    ret = deflate(&strm, Z_FINISH);
    deflateEnd(&strm);
    
    if (ret != Z_STREAM_END) {
        free(*compressed_data);
        return ret;
    }
    
    *compressed_len = buf_size - strm.avail_out;
    return Z_OK;
}

static int skystreet_decompress(const unsigned char *compressed_data, size_t compressed_len,
                               unsigned char **data, size_t *data_len) {
    z_stream strm;
    int ret;
    unsigned char out[1024];
    
    // Initialize decompression stream
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    
    ret = inflateInit2(&strm, 31);
    if (ret != Z_OK)
        return ret;
    
    // Prepare output buffer
    size_t out_size = 0;
    *data = NULL;
    
    // Set input
    strm.avail_in = compressed_len;
    strm.next_in = (unsigned char*)compressed_data;
    
    // Decompress in chunks of 1024 bytes
    do {
        strm.avail_out = sizeof(out);
        strm.next_out = out;
        
        ret = inflate(&strm, Z_NO_FLUSH);
        
        if (ret != Z_OK && ret != Z_STREAM_END) {
            inflateEnd(&strm);
            free(*data);
            return ret;
        }
        
        size_t have = sizeof(out) - strm.avail_out;
        *data = realloc(*data, out_size + have);
        if (!*data) {
            inflateEnd(&strm);
            return Z_MEM_ERROR;
        }
        
        memcpy(*data + out_size, out, have);
        out_size += have;
        
    } while (strm.avail_out == 0);
    
    inflateEnd(&strm);
    
    if (ret != Z_STREAM_END) {
        free(*data);
        return Z_DATA_ERROR;
    }
    
    *data_len = out_size;
    return Z_OK;
}

// === AREA 7SREF FUNCTIONS ===

static int is_7sref_path(const char *path) {
    return (strncmp(path, "/fuse_dir/7sref/", 16) == 0);
}

static int extract_7sref_info(const char *path, char *area, char *filename) {
    const char *file_part = path + 16;
    
    // Cari underscore pertama yang memisahkan area dan nama file
    const char *underscore = strchr(file_part, '_');
    if (!underscore) {
        return 0;
    }
    
    // Salin bagian area
    size_t area_len = underscore - file_part;
    strncpy(area, file_part, area_len);
    area[area_len] = '\0';
    
    // Salin bagian nama file
    strcpy(filename, underscore + 1);
    
    return 1;
}

static int transform_7sref_path(const char *path, char *target_path) {
    char area[PATH_MAX];
    char filename[PATH_MAX];
    
    if (!extract_7sref_info(path, area, filename)) {
        return 0;
    }
    
    // Bentuk path target di area yang sesuai
    sprintf(target_path, "/fuse_dir/%s/%s", area, filename);
    return 1;
}

// Memastikan path lengkap untuk akses ke sistem file nyata
static void maimai_fullpath(char fullpath[PATH_MAX], const char *path) {
    strcpy(fullpath, maimai_root_dir);
    strcat(fullpath, path);
}

// mendapatkan atribut file/direktori
static int maimai_getattr(const char *path, struct stat *stbuf) {
    memset(stbuf, 0, sizeof(struct stat));
    
    // Cek apakah ini adalah root direktori
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }
    
    // Cek direktori utama
    if (strcmp(path, "/chiho") == 0 || strcmp(path, "/fuse_dir") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }
    
    // Array area untuk menyederhanakan pengecekan
    const char *areas[] = {"starter", "metro", "dragon", "blackrose", "heaven", "skystreet", "youth"};
    int num_areas = 7;
    
    // Cek apakah path ada di dalam 
    for (int i = 0; i < num_areas; i++) {
        char area_path[100];
        sprintf(area_path, "/chiho/%s", areas[i]);
        if (strcmp(path, area_path) == 0) {
            stbuf->st_mode = S_IFDIR | 0755;
            stbuf->st_nlink = 2;
            return 0;
        }
        
        sprintf(area_path, "/fuse_dir/%s", areas[i]);
        if (strcmp(path, area_path) == 0) {
            stbuf->st_mode = S_IFDIR | 0755;
            stbuf->st_nlink = 2;
            return 0;
        }
    }
    
    // Cek special 7sref di fuse_dir
    if (strcmp(path, "/fuse_dir/7sref") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }
    
    if (is_starter_path(path)) {
        char transformed_path[PATH_MAX];
        transform_starter_path(transformed_path, path);
        
        int res = lstat(transformed_path, stbuf);
        if (res == -1)
            return -errno;
            
        return 0;
    }
   
    if (is_metro_path(path)) {
        char transformed_path[PATH_MAX];
        transform_metro_path(transformed_path, path);
        
        int res = lstat(transformed_path, stbuf);
        if (res == -1)
            return -errno;
            
        return 0;
    }
    
    if (is_dragon_path(path)) {
        char transformed_path[PATH_MAX];
        transform_dragon_path(transformed_path, path);
        
        int res = lstat(transformed_path, stbuf);
        if (res == -1)
            return -errno;
            
        return 0;
    }
    
    if (is_blackrose_path(path)) {
        char transformed_path[PATH_MAX];
        transform_blackrose_path(transformed_path, path);
        
        int res = lstat(transformed_path, stbuf);
        if (res == -1)
            return -errno;
            
        return 0;
    }
 
    if (is_heaven_path(path)) {
        char transformed_path[PATH_MAX];
        transform_heaven_path(transformed_path, path);
        
        int res = lstat(transformed_path, stbuf);
        if (res == -1)
            return -errno;
            
        return 0;
    }
    
    if (is_skystreet_path(path)) {
        char transformed_path[PATH_MAX];
        transform_skystreet_path(transformed_path, path);
        
        int res = lstat(transformed_path, stbuf);
        if (res == -1)
            return -errno;
            
        return 0;
    }
  
    if (is_7sref_path(path)) {
        char target_path[PATH_MAX];
        
        if (transform_7sref_path(path, target_path)) {
            return maimai_getattr(target_path, stbuf);
        } else {
            return -ENOENT;
        }
    }

    char fullpath[PATH_MAX];
    maimai_fullpath(fullpath, path);
    
    int res = lstat(fullpath, stbuf);
    if (res == -1)
        return -errno;
        
    return 0;
}

// membaca isi direktori
static int maimai_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                          off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;
    
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    
    // Cek root direktori
    if (strcmp(path, "/") == 0) {
        filler(buf, "chiho", NULL, 0);
        filler(buf, "fuse_dir", NULL, 0);
        return 0;
    }
    
    // Cek direktori chiho
    if (strcmp(path, "/chiho") == 0) {
        filler(buf, "starter", NULL, 0);
        filler(buf, "metro", NULL, 0);
        filler(buf, "dragon", NULL, 0);
        filler(buf, "blackrose", NULL, 0);
        filler(buf, "heaven", NULL, 0);
        filler(buf, "skystreet", NULL, 0);
        filler(buf, "youth", NULL, 0);
        return 0;
    }
    
    // Cek direktori fuse_dir
    if (strcmp(path, "/fuse_dir") == 0) {
        filler(buf, "starter", NULL, 0);
        filler(buf, "metro", NULL, 0);
        filler(buf, "dragon", NULL, 0);
        filler(buf, "blackrose", NULL, 0);
        filler(buf, "heaven", NULL, 0);
        filler(buf, "skystreet", NULL, 0);
        filler(buf, "youth", NULL, 0);
        filler(buf, "7sref", NULL, 0);
        return 0;
    }

    if (strcmp(path, "/fuse_dir/starter") == 0) {
        char fullpath[PATH_MAX];
        sprintf(fullpath, "%s/chiho/starter", maimai_root_dir);
        
        DIR *dp = opendir(fullpath);
        if (dp == NULL)
            return -errno;
            
        struct dirent *de;
        while ((de = readdir(dp)) != NULL) {
            if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
                continue;

            char filename[PATH_MAX];
            strcpy(filename, de->d_name);
            char *dot = strrchr(filename, '.');
            if (dot && strcmp(dot, ".mai") == 0) {
                *dot = '\0';
                
                struct stat st;
                memset(&st, 0, sizeof(st));
                st.st_ino = de->d_ino;
                st.st_mode = S_IFREG | 0644;
                
                if (filler(buf, filename, &st, 0))
                    break;
            }
        }
        
        closedir(dp);
        return 0;
    }
 
    if (strcmp(path, "/fuse_dir/metro") == 0) {
        char fullpath[PATH_MAX];
        sprintf(fullpath, "%s/chiho/metro", maimai_root_dir);
        
        DIR *dp = opendir(fullpath);
        if (dp == NULL)
            return -errno;
            
        struct dirent *de;
        while ((de = readdir(dp)) != NULL) {
            if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
                continue;
                
            char filename[PATH_MAX];
            strcpy(filename, de->d_name);
            char *dot = strrchr(filename, '.');
            if (dot && strcmp(dot, ".ccc") == 0) {
                *dot = '\0';
                
                struct stat st;
                memset(&st, 0, sizeof(st));
                st.st_ino = de->d_ino;
                st.st_mode = S_IFREG | 0644; 
                
                if (filler(buf, filename, &st, 0))
                    break;
            }
        }
        
        closedir(dp);
        return 0;
    }
    
    if (strcmp(path, "/fuse_dir/dragon") == 0) {
        char fullpath[PATH_MAX];
        sprintf(fullpath, "%s/chiho/dragon", maimai_root_dir);
        
        DIR *dp = opendir(fullpath);
        if (dp == NULL)
            return -errno;
            
        struct dirent *de;
        while ((de = readdir(dp)) != NULL) {
            if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
                continue;

            char filename[PATH_MAX];
            strcpy(filename, de->d_name);
            char *dot = strrchr(filename, '.');
            if (dot && strcmp(dot, ".rot") == 0) {
                *dot = '\0';
                
                struct stat st;
                memset(&st, 0, sizeof(st));
                st.st_ino = de->d_ino;
                st.st_mode = S_IFREG | 0644;
                
                if (filler(buf, filename, &st, 0))
                    break;
            }
        }
        
        closedir(dp);
        return 0;
    }
    
    if (strcmp(path, "/fuse_dir/blackrose") == 0) {
        char fullpath[PATH_MAX];
        sprintf(fullpath, "%s/chiho/blackrose", maimai_root_dir);
        
        DIR *dp = opendir(fullpath);
        if (dp == NULL)
            return -errno;
            
        struct dirent *de;
        while ((de = readdir(dp)) != NULL) {
            if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
                continue;
                
            char filename[PATH_MAX];
            strcpy(filename, de->d_name);
            char *dot = strrchr(filename, '.');
            if (dot && strcmp(dot, ".bin") == 0) {
                *dot = '\0';
                
                struct stat st;
                memset(&st, 0, sizeof(st));
                st.st_ino = de->d_ino;
                st.st_mode = S_IFREG | 0644;
                
                if (filler(buf, filename, &st, 0))
                    break;
            }
        }
        
        closedir(dp);
        return 0;
    }
    
    if (strcmp(path, "/fuse_dir/heaven") == 0) {
        char fullpath[PATH_MAX];
        sprintf(fullpath, "%s/chiho/heaven", maimai_root_dir);
        
        DIR *dp = opendir(fullpath);
        if (dp == NULL)
            return -errno;
            
        struct dirent *de;
        while ((de = readdir(dp)) != NULL) {
            if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
                continue;

            char filename[PATH_MAX];
            strcpy(filename, de->d_name);
            char *dot = strrchr(filename, '.');
            if (dot && strcmp(dot, ".enc") == 0) {
                *dot = '\0';
                
                struct stat st;
                memset(&st, 0, sizeof(st));
                st.st_ino = de->d_ino;
                st.st_mode = S_IFREG | 0644;
                
                if (filler(buf, filename, &st, 0))
                    break;
            }
        }
        
        closedir(dp);
        return 0;
    }
    
    if (strcmp(path, "/fuse_dir/skystreet") == 0) {
        char fullpath[PATH_MAX];
        sprintf(fullpath, "%s/chiho/skystreet", maimai_root_dir);
        
        DIR *dp = opendir(fullpath);
        if (dp == NULL)
            return -errno;
            
        struct dirent *de;
        while ((de = readdir(dp)) != NULL) {
            if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
                continue;

            char filename[PATH_MAX];
            strcpy(filename, de->d_name);
            char *dot = strrchr(filename, '.');
            if (dot && strcmp(dot, ".gz") == 0) {
                *dot = '\0';
                
                struct stat st;
                memset(&st, 0, sizeof(st));
                st.st_ino = de->d_ino;
                st.st_mode = S_IFREG | 0644;
                
                if (filler(buf, filename, &st, 0))
                    break;
            }
        }
        
        closedir(dp);
        return 0;
    }
    
    if (strcmp(path, "/fuse_dir/7sref") == 0) {
        
        const char *areas[] = {"starter", "metro", "dragon", "blackrose", "heaven", "skystreet", "youth"};
        int num_areas = 7;
        
        // Scan semua area dan tambahkan file dengan prefix area
        for (int i = 0; i < num_areas; i++) {
            char area_path[PATH_MAX];
            sprintf(area_path, "%s/fuse_dir/%s", maimai_root_dir, areas[i]);
            
            DIR *dp = opendir(area_path);
            if (dp == NULL)
                continue;
                
            struct dirent *de;
            while ((de = readdir(dp)) != NULL) {
                if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
                    continue;
                    
                // Buat nama file dengan format: area_nama_file
                char ref_name[PATH_MAX];
                sprintf(ref_name, "%s_%s", areas[i], de->d_name);
                
                struct stat st;
                memset(&st, 0, sizeof(st));
                st.st_ino = de->d_ino;
                
                if (de->d_type == DT_DIR)
                    st.st_mode = S_IFDIR | 0755;
                else
                    st.st_mode = S_IFREG | 0644;
                    
                if (filler(buf, ref_name, &st, 0))
                    break;
            }
            
            closedir(dp);
        }
        
        return 0;
    }
    
    char fullpath[PATH_MAX];
    maimai_fullpath(fullpath, path);
    
    DIR *dp = opendir(fullpath);
    if (dp == NULL)
        return -errno;
        
    struct dirent *de;
    while ((de = readdir(dp)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
            continue;
            
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        
        if (de->d_type == DT_DIR)
            st.st_mode = S_IFDIR | 0755;
        else
            st.st_mode = S_IFREG | 0644;
            
        if (filler(buf, de->d_name, &st, 0))
            break;
    }
    
    closedir(dp);
    return 0;
}

// membaca isi file
static int maimai_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi) {
    (void) fi;

    if (is_starter_path(path)) {
        char transformed_path[PATH_MAX];
        transform_starter_path(transformed_path, path);
        
        int fd = open(transformed_path, O_RDONLY);
        if (fd == -1)
            return -errno;
            
        int res = pread(fd, buf, size, offset);
        if (res == -1) {
            res = -errno;
            close(fd);
            return res;
        }
        
        close(fd);
        return res;
    }
    
    if (is_metro_path(path)) {
        char transformed_path[PATH_MAX];
        transform_metro_path(transformed_path, path);
        
        int fd = open(transformed_path, O_RDONLY);
        if (fd == -1)
            return -errno;

        char *temp_buf = malloc(size);
        if (!temp_buf) {
            close(fd);
            return -ENOMEM;
        }

        int res = pread(fd, temp_buf, size, offset);
        if (res == -1) {
            res = -errno;
            free(temp_buf);
            close(fd);
            return res;
        }
        metro_decrypt(temp_buf, buf, res);
        
        free(temp_buf);
        close(fd);
        return res;
    }
    
    if (is_dragon_path(path)) {
        char transformed_path[PATH_MAX];
        transform_dragon_path(transformed_path, path);
        
        int fd = open(transformed_path, O_RDONLY);
        if (fd == -1)
            return -errno;

        char *temp_buf = malloc(size);
        if (!temp_buf) {
            close(fd);
            return -ENOMEM;
        }

        int res = pread(fd, temp_buf, size, offset);
        if (res == -1) {
            res = -errno;
            free(temp_buf);
            close(fd);
            return res;
        }
        
        rot13(temp_buf, buf, res);
        
        free(temp_buf);
        close(fd);
        return res;
    }
    
    if (is_blackrose_path(path)) {
        char transformed_path[PATH_MAX];
        transform_blackrose_path(transformed_path, path);
        
        int fd = open(transformed_path, O_RDONLY);
        if (fd == -1)
            return -errno;

        int res = pread(fd, buf, size, offset);
        if (res == -1) {
            res = -errno;
            close(fd);
            return res;
        }
        
        close(fd);
        return res;
    }
    
    if (is_heaven_path(path)) {
        char transformed_path[PATH_MAX];
        transform_heaven_path(transformed_path, path);
        
        int fd = open(transformed_path, O_RDONLY);
        if (fd == -1)
            return -errno;
        
        // Cari ukuran file
        struct stat st;
        if (fstat(fd, &st) == -1) {
            close(fd);
            return -errno;
        }
        
        unsigned char *encrypted_data = malloc(st.st_size);
        if (!encrypted_data) {
            close(fd);
            return -ENOMEM;
        }
      
        ssize_t bytes_read = read(fd, encrypted_data, st.st_size);
        close(fd);
        
        if (bytes_read != st.st_size) {
            free(encrypted_data);
            return -EIO;
        }
        
        unsigned char *decrypted_data = malloc(st.st_size);
        if (!decrypted_data) {
            free(encrypted_data);
            return -ENOMEM;
        }
        
        int decrypted_len = 0;
        if (!heaven_decrypt(encrypted_data, bytes_read, decrypted_data, &decrypted_len)) {
            free(encrypted_data);
            free(decrypted_data);
            return -EIO;
        }
        
        if (offset < decrypted_len) {
            if (offset + size > decrypted_len)
                size = decrypted_len - offset;
            memcpy(buf, decrypted_data + offset, size);
        } else {
            size = 0;
        }
        
        free(encrypted_data);
        free(decrypted_data);
        return size;
    }
    
    if (is_skystreet_path(path)) {
        char transformed_path[PATH_MAX];
        transform_skystreet_path(transformed_path, path);
        
        int fd = open(transformed_path, O_RDONLY);
        if (fd == -1)
            return -errno;
      
        struct stat st;
        if (fstat(fd, &st) == -1) {
            close(fd);
            return -errno;
        }
        
        unsigned char *compressed_data = malloc(st.st_size);
        if (!compressed_data) {
            close(fd);
            return -ENOMEM;
        }
        
        ssize_t bytes_read = read(fd, compressed_data, st.st_size);
        close(fd);
        
        if (bytes_read != st.st_size) {
            free(compressed_data);
            return -EIO;
        }
        
        unsigned char *decompressed_data = NULL;
        size_t decompressed_len = 0;
        
        int ret = skystreet_decompress(compressed_data, bytes_read, 
                                     &decompressed_data, &decompressed_len);
        
        free(compressed_data);
        
        if (ret != Z_OK) {
            if (decompressed_data)
                free(decompressed_data);
            return -EIO;
        }
        
        if (offset < decompressed_len) {
            if (offset + size > decompressed_len)
                size = decompressed_len - offset;
            memcpy(buf, decompressed_data + offset, size);
        } else {
            size = 0;
        }
        
        free(decompressed_data);
        return size;
    }
    
    if (is_7sref_path(path)) {
        char target_path[PATH_MAX];
        if (transform_7sref_path(path, target_path)) {
            return maimai_read(target_path, buf, size, offset, fi);
        } else {
            return -ENOENT;
        }
    }
    
    char fullpath[PATH_MAX];
    maimai_fullpath(fullpath, path);
    
    int fd = open(fullpath, O_RDONLY);
    if (fd == -1)
        return -errno;
        
    int res = pread(fd, buf, size, offset);
    if (res == -1)
        res = -errno;
        
    close(fd);
    return res;
}

// menulis ke file
static int maimai_write(const char *path, const char *buf, size_t size,
                       off_t offset, struct fuse_file_info *fi) {
    (void) fi;
    
    if (is_starter_path(path)) {
        char transformed_path[PATH_MAX];
        transform_starter_path(transformed_path, path);
        
        int fd = open(transformed_path, O_WRONLY);
        if (fd == -1)
            return -errno;
            
        int res = pwrite(fd, buf, size, offset);
        if (res == -1) {
            res = -errno;
            close(fd);
            return res;
        }
        
        close(fd);
        return res;
    }
    
    if (is_metro_path(path)) {
        char transformed_path[PATH_MAX];
        transform_metro_path(transformed_path, path);
        
        int fd = open(transformed_path, O_WRONLY);
        if (fd == -1)
            return -errno;
            
        char *temp_buf = malloc(size);
        if (!temp_buf) {
            close(fd);
            return -ENOMEM;
        }
        
        metro_encrypt(buf, temp_buf, size);
        
        int res = pwrite(fd, temp_buf, size, offset);
        if (res == -1)
            res = -errno;
            
        free(temp_buf);
        close(fd);
        return res;
    }

    if (is_dragon_path(path)) {
        char transformed_path[PATH_MAX];
        transform_dragon_path(transformed_path, path);
        
        int fd = open(transformed_path, O_WRONLY);
        if (fd == -1)
            return -errno;

        char *temp_buf = malloc(size);
        if (!temp_buf) {
            close(fd);
            return -ENOMEM;
        }
        
        rot13(buf, temp_buf, size);

        int res = pwrite(fd, temp_buf, size, offset);
        if (res == -1)
            res = -errno;
            
        free(temp_buf);
        close(fd);
        return res;
    }
    if (is_blackrose_path(path)) {
        char transformed_path[PATH_MAX];
        transform_blackrose_path(transformed_path, path);
        
        int fd = open(transformed_path, O_WRONLY);
        if (fd == -1)
            return -errno;

        int res = pwrite(fd, buf, size, offset);
        if (res == -1) {
            res = -errno;
            close(fd);
            return res;
        }
        
        close(fd);
        return res;
    }

    if (is_heaven_path(path)) {
        char transformed_path[PATH_MAX];
        transform_heaven_path(transformed_path, path);
        
        int fd = open(transformed_path, O_RDWR | O_CREAT, 0644);
        if (fd == -1)
            return -errno;

        struct stat st;
        if (fstat(fd, &st) == -1) {
            close(fd);
            return -errno;
        }
        
        size_t total_size = offset + size;
        unsigned char *plaintext = NULL;
        
        if (st.st_size > 0 && offset > 0) {
            unsigned char *encrypted_data = malloc(st.st_size);
            if (!encrypted_data) {
                close(fd);
                return -ENOMEM;
            }
            
            ssize_t bytes_read = read(fd, encrypted_data, st.st_size);
            
            if (bytes_read != st.st_size) {
                free(encrypted_data);
                close(fd);
                return -EIO;
            }
            
            unsigned char *decrypted_data = malloc(st.st_size);
            if (!decrypted_data) {
                free(encrypted_data);
                close(fd);
                return -ENOMEM;
            }
        
            int decrypted_len = 0;
            if (!heaven_decrypt(encrypted_data, bytes_read, decrypted_data, &decrypted_len)) {
                free(encrypted_data);
                free(decrypted_data);
                close(fd);
                return -EIO;
            }
            
            free(encrypted_data);

            plaintext = malloc(total_size > decrypted_len ? total_size : decrypted_len);
            if (!plaintext) {
                free(decrypted_data);
                close(fd);
                return -ENOMEM;
            }

            memcpy(plaintext, decrypted_data, decrypted_len);
            free(decrypted_data);

            if (total_size > decrypted_len)
                memset(plaintext + decrypted_len, 0, total_size - decrypted_len);
        } else {
            plaintext = malloc(total_size);
            if (!plaintext) {
                close(fd);
                return -ENOMEM;
            }
            memset(plaintext, 0, total_size);
        }
        memcpy(plaintext + offset, buf, size);
        
        unsigned char *ciphertext = malloc(total_size + AES_BLOCK_SIZE + AES_BLOCK_SIZE);
        if (!ciphertext) {
            free(plaintext);
            close(fd);
            return -ENOMEM;
        }
        
        int ciphertext_len = 0;
        if (!heaven_encrypt(plaintext, total_size, ciphertext, &ciphertext_len)) {
            free(plaintext);
            free(ciphertext);
            close(fd);
            return -EIO;
        }
        
        free(plaintext);

        lseek(fd, 0, SEEK_SET);
        if (ftruncate(fd, 0) == -1) {
            free(ciphertext);
            close(fd);
            return -errno;
        }
        
        ssize_t bytes_written = write(fd, ciphertext, ciphertext_len);
        free(ciphertext);
        close(fd);
        
        if (bytes_written != ciphertext_len)
            return -EIO;
            
        return size;
    }
    
    if (is_skystreet_path(path)) {
        char transformed_path[PATH_MAX];
        transform_skystreet_path(transformed_path, path);
        
        unsigned char *decompressed_data = NULL;
        size_t decompressed_len = 0;

        int fd = open(transformed_path, O_RDONLY);
        if (fd >= 0) {
            struct stat st;
            if (fstat(fd, &st) == -1) {
                close(fd);
                return -errno;
            }
         
            unsigned char *compressed_data = malloc(st.st_size);
            if (!compressed_data) {
                close(fd);
                return -ENOMEM;
            }
   
            ssize_t bytes_read = read(fd, compressed_data, st.st_size);
            close(fd);
            
            if (bytes_read != st.st_size) {
                free(compressed_data);
                return -EIO;
            }
       
            int ret = skystreet_decompress(compressed_data, bytes_read, 
                                         &decompressed_data, &decompressed_len);
            
            free(compressed_data);
            
            if (ret != Z_OK) {
                if (decompressed_data)
                    free(decompressed_data);
                return -EIO;
            }
        }
        
        size_t new_size = offset + size;
        if (decompressed_len < new_size) {
            unsigned char *new_data = realloc(decompressed_data, new_size);
            if (!new_data) {
                if (decompressed_data)
                    free(decompressed_data);
                return -ENOMEM;
            }
            
            if (!decompressed_data)
                memset(new_data, 0, new_size);
                
            decompressed_data = new_data;
            decompressed_len = new_size;
        }
        
       
        memcpy(decompressed_data + offset, buf, size);
        
        unsigned char *compressed_data = NULL;
        size_t compressed_len = 0;
        
        int ret = skystreet_compress(decompressed_data, decompressed_len,
                                   &compressed_data, &compressed_len);
        
        free(decompressed_data);
        
        if (ret != Z_OK) {
            if (compressed_data)
                free(compressed_data);
            return -EIO;
        }
       
        fd = open(transformed_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            free(compressed_data);
            return -errno;
        }
        
        ssize_t bytes_written = write(fd, compressed_data, compressed_len);
        close(fd);
        free(compressed_data);
        
        if (bytes_written != compressed_len)
            return -EIO;
            
        return size;
    }
  
    if (is_7sref_path(path)) {
        char target_path[PATH_MAX];
        if (transform_7sref_path(path, target_path)) {
            return maimai_write(target_path, buf, size, offset, fi);
        } else {
            return -ENOENT;
        }
    }
    
    char fullpath[PATH_MAX];
    maimai_fullpath(fullpath, path);
    
    int fd = open(fullpath, O_WRONLY);
    if (fd == -1)
        return -errno;
        
    int res = pwrite(fd, buf, size, offset);
    if (res == -1)
        res = -errno;
        
    close(fd);
    return res;
}

static int maimai_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    // Penanganan khusus untuk area starter
    if (is_starter_path(path)) {
        char transformed_path[PATH_MAX];
        transform_starter_path(transformed_path, path);
        
        int fd = open(transformed_path, O_CREAT | O_EXCL | O_WRONLY, mode);
        if (fd == -1)
            return -errno;
            
        fi->fh = fd;
        return 0;
    }
    
    if (is_metro_path(path)) {
        char transformed_path[PATH_MAX];
        transform_metro_path(transformed_path, path);
        
        int fd = open(transformed_path, O_CREAT | O_EXCL | O_WRONLY, mode);
        if (fd == -1)
            return -errno;
            
        fi->fh = fd;
        return 0;
    }
    
    if (is_dragon_path(path)) {
        char transformed_path[PATH_MAX];
        transform_dragon_path(transformed_path, path);
        
        int fd = open(transformed_path, O_CREAT | O_EXCL | O_WRONLY, mode);
        if (fd == -1)
            return -errno;
            
        fi->fh = fd;
        return 0;
    }
    
    if (is_blackrose_path(path)) {
        char transformed_path[PATH_MAX];
        transform_blackrose_path(transformed_path, path);
        
        int fd = open(transformed_path, O_CREAT | O_EXCL | O_WRONLY, mode);
        if (fd == -1)
            return -errno;
            
        fi->fh = fd;
        return 0;
    }
    
    if (is_heaven_path(path)) {
        char transformed_path[PATH_MAX];
        transform_heaven_path(transformed_path, path);
        
        int fd = open(transformed_path, O_CREAT | O_WRONLY, mode);
        if (fd == -1)
            return -errno;
            
        close(fd);
        fi->fh = 0;
        return 0;
    }
    
    if (is_skystreet_path(path)) {
        char transformed_path[PATH_MAX];
        transform_skystreet_path(transformed_path, path);
      
        unsigned char empty[] = "";
        unsigned char *compressed_data = NULL;
        size_t compressed_len = 0;
        
        int ret = skystreet_compress(empty, 0, &compressed_data, &compressed_len);
        if (ret != Z_OK)
            return -EIO;
        
        int fd = open(transformed_path, O_CREAT | O_WRONLY, mode);
        if (fd == -1) {
            free(compressed_data);
            return -errno;
        }
        
        write(fd, compressed_data, compressed_len);
        close(fd);
        free(compressed_data);
        
        fi->fh = 0;
        return 0;
    }
    
    if (is_7sref_path(path)) {
        char target_path[PATH_MAX];
        if (transform_7sref_path(path, target_path)) {
            return maimai_create(target_path, mode, fi);
        } else {
            return -ENOENT;
        }
    }
    
    char fullpath[PATH_MAX];
    maimai_fullpath(fullpath, path);
    
    int fd = open(fullpath, O_CREAT | O_EXCL | O_WRONLY, mode);
    if (fd == -1)
        return -errno;
        
    fi->fh = fd;
    return 0;
}

// menghapus file
static int maimai_unlink(const char *path) {
    if (is_starter_path(path)) {
        char transformed_path[PATH_MAX];
        transform_starter_path(transformed_path, path);
        
        int res = unlink(transformed_path);
        if (res == -1)
            return -errno;
            
        return 0;
    }
    
    if (is_metro_path(path)) {
        char transformed_path[PATH_MAX];
        transform_metro_path(transformed_path, path);
        
        int res = unlink(transformed_path);
        if (res == -1)
            return -errno;
            
        return 0;
    }
    
    if (is_dragon_path(path)) {
        char transformed_path[PATH_MAX];
        transform_dragon_path(transformed_path, path);
        
        int res = unlink(transformed_path);
        if (res == -1)
            return -errno;
            
        return 0;
    }
    
    if (is_blackrose_path(path)) {
        char transformed_path[PATH_MAX];
        transform_blackrose_path(transformed_path, path);
        
        int res = unlink(transformed_path);
        if (res == -1)
            return -errno;
            
        return 0;
    }
    
    if (is_heaven_path(path)) {
        char transformed_path[PATH_MAX];
        transform_heaven_path(transformed_path, path);
        
        int res = unlink(transformed_path);
        if (res == -1)
            return -errno;
            
        return 0;
    }
    if (is_skystreet_path(path)) {
        char transformed_path[PATH_MAX];
        transform_skystreet_path(transformed_path, path);
        
        int res = unlink(transformed_path);
        if (res == -1)
            return -errno;
            
        return 0;
    }
    
    if (is_7sref_path(path)) {
        char target_path[PATH_MAX];
        if (transform_7sref_path(path, target_path)) {
            return maimai_unlink(target_path);
        } else {
            return -ENOENT;
        }
    }
    
    char fullpath[PATH_MAX];
    maimai_fullpath(fullpath, path);
    
    int res = unlink(fullpath);
    if (res == -1)
        return -errno;
        
    return 0;
}

static struct fuse_operations maimai_oper = {
    .getattr = maimai_getattr,
    .readdir = maimai_readdir,
    .read = maimai_read,
    .write = maimai_write,
    .create = maimai_create,
    .unlink = maimai_unlink,
};

static void init_maimai_fs() {
    mkdir(maimai_root_dir, 0755);

    sprintf(chiho_dir, "%s/chiho", maimai_root_dir);
    sprintf(fuse_dir, "%s/fuse_dir", maimai_root_dir);
    
    mkdir(chiho_dir, 0755);
    
    const char *areas[] = {"starter", "metro", "dragon", "blackrose", "heaven", "skystreet", "youth"};
    int num_areas = 7;
    
    for (int i = 0; i < num_areas; i++) {
        char path[PATH_MAX];
        sprintf(path, "%s/%s", chiho_dir, areas[i]);
        mkdir(path, 0755);
    }

    mkdir(fuse_dir, 0755);
    
    for (int i = 0; i < num_areas; i++) {
        char path[PATH_MAX];
        sprintf(path, "%s/%s", fuse_dir, areas[i]);
        mkdir(path, 0755);
    }
    
    char path[PATH_MAX];
    sprintf(path, "%s/7sref", fuse_dir);
    mkdir(path, 0755);
}

int main(int argc, char *argv[]) {
    init_maimai_fs();
    return fuse_main(argc, argv, &maimai_oper, NULL);
}
```
### Pembuatan fuse
Sebelum FUSE berjalan, fungsi init_maimai_fs() akan membuat seluruh direktori yang diperlukan untuk menyimpan data backend dan direktori virtual yang akan di-mount.
```c
static void init_maimai_fs() {
    mkdir(maimai_root_dir, 0755);
    sprintf(chiho_dir, "%s/chiho", maimai_root_dir);
    sprintf(fuse_dir, "%s/fuse_dir", maimai_root_dir);
    mkdir(chiho_dir, 0755);

    const char *areas[] = {"starter", "metro", "dragon", "blackrose", "heaven", "skystreet", "youth"};
    int num_areas = 7;
    for (int i = 0; i < num_areas; i++) {
        char path[PATH_MAX];
        sprintf(path, "%s/%s", chiho_dir, areas[i]);
        mkdir(path, 0755);
    }

    mkdir(fuse_dir, 0755);
    for (int i = 0; i < num_areas; i++) {
        char path[PATH_MAX];
        sprintf(path, "%s/%s", fuse_dir, areas[i]);
        mkdir(path, 0755);
    }

    char path[PATH_MAX];
    sprintf(path, "%s/7sref", fuse_dir);
    mkdir(path, 0755);
}

```
Setelah struktur direktori siap, program mendefinisikan operasi-operasi yang akan di-handle oleh FUSE melalui struct fuse_operations. 
```c
static struct fuse_operations maimai_oper = {
    .getattr = maimai_getattr,
    .readdir = maimai_readdir,
    .read = maimai_read,
    .write = maimai_write,
    .create = maimai_create,
    .unlink = maimai_unlink,
};

```
### Operasi getattr
Fungsi maimai_getattr bertanggung jawab untuk mendapatkan atribut dari file atau direktori.
```c
static int maimai_getattr(const char *path, struct stat *stbuf) {
    memset(stbuf, 0, sizeof(struct stat));
    
    // Pengecekan root direktori
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }
    
    // Pengecekan direktori utama
    if (strcmp(path, "/chiho") == 0 || strcmp(path, "/fuse_dir") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }
    
    // Pengecekan area-area
    const char *areas[] = {"starter", "metro", "dragon", "blackrose", "heaven", "skystreet", "youth"};
    int num_areas = 7;
    
    // Pengecekan path di setiap area
    for (int i = 0; i < num_areas; i++) {
        char area_path[100];
        sprintf(area_path, "/chiho/%s", areas[i]);
        if (strcmp(path, area_path) == 0) {
            stbuf->st_mode = S_IFDIR | 0755;
            stbuf->st_nlink = 2;
            return 0;
        }
        
        sprintf(area_path, "/fuse_dir/%s", areas[i]);
        if (strcmp(path, area_path) == 0) {
            stbuf->st_mode = S_IFDIR | 0755;
            stbuf->st_nlink = 2;
            return 0;
        }
    }
    
    // Pengecekan 7sref
    if (strcmp(path, "/fuse_dir/7sref") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }
    
    // Pengecekan file di area-area spesifik
    if (is_starter_path(path)) {
        char transformed_path[PATH_MAX];
        transform_starter_path(transformed_path, path);
        int res = lstat(transformed_path, stbuf);
        if (res == -1)
            return -errno;
        return 0;
    }
    
    // Pengecekan serupa untuk area-area lain...
    
    // Pengecekan 7sref path
    if (is_7sref_path(path)) {
        char target_path[PATH_MAX];
        if (transform_7sref_path(path, target_path)) {
            return maimai_getattr(target_path, stbuf);
        } else {
            return -ENOENT;
        }
    }
    
    // Path default
    char fullpath[PATH_MAX];
    maimai_fullpath(fullpath, path);
    int res = lstat(fullpath, stbuf);
    if (res == -1)
        return -errno;
    return 0;
}

```
### Operasi readdir
```c
static int maimai_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;
    
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    
    // Pengecekan root direktori
    if (strcmp(path, "/") == 0) {
        filler(buf, "chiho", NULL, 0);
        filler(buf, "fuse_dir", NULL, 0);
        return 0;
    }
    
    // Pengecekan direktori chiho
    if (strcmp(path, "/chiho") == 0) {
        filler(buf, "starter", NULL, 0);
        filler(buf, "metro", NULL, 0);
        // ... area lainnya
        return 0;
    }
    
    // Pengecekan direktori fuse_dir
    if (strcmp(path, "/fuse_dir") == 0) {
        filler(buf, "starter", NULL, 0);
        filler(buf, "metro", NULL, 0);
        // ... area lainnya
        filler(buf, "7sref", NULL, 0);
        return 0;
    }
    
    // Pengecekan area starter
    if (strcmp(path, "/fuse_dir/starter") == 0) {
        char fullpath[PATH_MAX];
        sprintf(fullpath, "%s/chiho/starter", maimai_root_dir);
        DIR *dp = opendir(fullpath);
        if (dp == NULL)
            return -errno;
            
        struct dirent *de;
        while ((de = readdir(dp)) != NULL) {
            if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
                continue;
                
            char filename[PATH_MAX];
            strcpy(filename, de->d_name);
            char *dot = strrchr(filename, '.');
            if (dot && strcmp(dot, ".mai") == 0) {
                *dot = '\0';  // Menghilangkan ekstensi .mai
                struct stat st;
                memset(&st, 0, sizeof(st));
                st.st_ino = de->d_ino;
                st.st_mode = S_IFREG | 0644;
                if (filler(buf, filename, &st, 0))
                    break;
            }
        }
        closedir(dp);
        return 0;
    }
    
    // Pengecekan serupa untuk area-area lain...
    
    // Pengecekan 7sref
    if (strcmp(path, "/fuse_dir/7sref") == 0) {
        const char *areas[] = {"starter", "metro", "dragon", "blackrose", "heaven", "skystreet", "youth"};
        int num_areas = 7;
        
        // Scan semua area dan tambahkan file dengan prefix area
        for (int i = 0; i < num_areas; i++) {
            char area_path[PATH_MAX];
            sprintf(area_path, "%s/fuse_dir/%s", maimai_root_dir, areas[i]);
            DIR *dp = opendir(area_path);
            if (dp == NULL)
                continue;
                
            struct dirent *de;
            while ((de = readdir(dp)) != NULL) {
                if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
                    continue;
                    
                // Buat nama file dengan format: area_nama_file
                char ref_name[PATH_MAX];
                sprintf(ref_name, "%s_%s", areas[i], de->d_name);
                struct stat st;
                memset(&st, 0, sizeof(st));
                st.st_ino = de->d_ino;
                if (de->d_type == DT_DIR)
                    st.st_mode = S_IFDIR | 0755;
                else
                    st.st_mode = S_IFREG | 0644;
                if (filler(buf, ref_name, &st, 0))
                    break;
            }
            closedir(dp);
        }
        return 0;
    }
    
    // Default path
    char fullpath[PATH_MAX];
    maimai_fullpath(fullpath, path);
    DIR *dp = opendir(fullpath);
    if (dp == NULL)
        return -errno;
        
    struct dirent *de;
    while ((de = readdir(dp)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
            continue;
            
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        if (de->d_type == DT_DIR)
            st.st_mode = S_IFDIR | 0755;
        else
            st.st_mode = S_IFREG | 0644;
        if (filler(buf, de->d_name, &st, 0))
            break;
    }
    closedir(dp);
    return 0;
}

```
### Operasi read
```c
static int maimai_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi) {
    (void) fi;
    
    // Area starter (tanpa transformasi konten)
    if (is_starter_path(path)) {
        char transformed_path[PATH_MAX];
        transform_starter_path(transformed_path, path);
        int fd = open(transformed_path, O_RDONLY);
        if (fd == -1)
            return -errno;
        int res = pread(fd, buf, size, offset);
        if (res == -1) {
            res = -errno;
            close(fd);
            return res;
        }
        close(fd);
        return res;
    }
    
    // Area metro (dengan dekripsi)
    if (is_metro_path(path)) {
        char transformed_path[PATH_MAX];
        transform_metro_path(transformed_path, path);
        int fd = open(transformed_path, O_RDONLY);
        if (fd == -1)
            return -errno;
        char *temp_buf = malloc(size);
        if (!temp_buf) {
            close(fd);
            return -ENOMEM;
        }
        int res = pread(fd, temp_buf, size, offset);
        if (res == -1) {
            res = -errno;
            free(temp_buf);
            close(fd);
            return res;
        }
        metro_decrypt(temp_buf, buf, res);
        free(temp_buf);
        close(fd);
        return res;
    }
    
    // Implementasi serupa untuk area-area lain...
    
    // Area heaven (dengan dekripsi AES)
    if (is_heaven_path(path)) {
        char transformed_path[PATH_MAX];
        transform_heaven_path(transformed_path, path);
        int fd = open(transformed_path, O_RDONLY);
        if (fd == -1)
            return -errno;
        
        struct stat st;
        if (fstat(fd, &st) == -1) {
            close(fd);
            return -errno;
        }
        
        unsigned char *encrypted_data = malloc(st.st_size);
        if (!encrypted_data) {
            close(fd);
            return -ENOMEM;
        }
        
        ssize_t bytes_read = read(fd, encrypted_data, st.st_size);
        close(fd);
        if (bytes_read != st.st_size) {
            free(encrypted_data);
            return -EIO;
        }
        
        unsigned char *decrypted_data = malloc(st.st_size);
        if (!decrypted_data) {
            free(encrypted_data);
            return -ENOMEM;
        }
        
        int decrypted_len = 0;
        if (!heaven_decrypt(encrypted_data, bytes_read, decrypted_data, &decrypted_len)) {
            free(encrypted_data);
            free(decrypted_data);
            return -EIO;
        }
        
        if (offset < decrypted_len) {
            if (offset + size > decrypted_len)
                size = decrypted_len - offset;
            memcpy(buf, decrypted_data + offset, size);
        } else {
            size = 0;
        }
        
        free(encrypted_data);
        free(decrypted_data);
        return size;
    }
    
    // Area 7sref (redirect)
    if (is_7sref_path(path)) {
        char target_path[PATH_MAX];
        if (transform_7sref_path(path, target_path)) {
            return maimai_read(target_path, buf, size, offset, fi);
        } else {
            return -ENOENT;
        }
    }
    
    // Default path
    char fullpath[PATH_MAX];
    maimai_fullpath(fullpath, path);
    int fd = open(fullpath, O_RDONLY);
    if (fd == -1)
        return -errno;
    int res = pread(fd, buf, size, offset);
    if (res == -1)
        res = -errno;
    close(fd);
    return res;
}

```
### Operasi write 
```c
static int maimai_write(const char *path, const char *buf, size_t size,
                       off_t offset, struct fuse_file_info *fi) {
    (void) fi;
    
    // Area starter (tanpa transformasi konten)
    if (is_starter_path(path)) {
        char transformed_path[PATH_MAX];
        transform_starter_path(transformed_path, path);
        int fd = open(transformed_path, O_WRONLY);
        if (fd == -1)
            return -errno;
        int res = pwrite(fd, buf, size, offset);
        if (res == -1) {
            res = -errno;
            close(fd);
            return res;
        }
        close(fd);
        return res;
    }
    
    // Area metro (dengan enkripsi)
    if (is_metro_path(path)) {
        char transformed_path[PATH_MAX];
        transform_metro_path(transformed_path, path);
        int fd = open(transformed_path, O_WRONLY);
        if (fd == -1)
            return -errno;
        char *temp_buf = malloc(size);
        if (!temp_buf) {
            close(fd);
            return -ENOMEM;
        }
        metro_encrypt(buf, temp_buf, size);
        int res = pwrite(fd, temp_buf, size, offset);
        if (res == -1)
            res = -errno;
        free(temp_buf);
        close(fd);
        return res;
    }
    
    // Implementasi serupa untuk area-area lain...
    
    // Area heaven (dengan enkripsi AES)
    if (is_heaven_path(path)) {
        char transformed_path[PATH_MAX];
        transform_heaven_path(transformed_path, path);
        int fd = open(transformed_path, O_RDWR | O_CREAT, 0644);
        if (fd == -1)
            return -errno;
            
        struct stat st;
        if (fstat(fd, &st) == -1) {
            close(fd);
            return -errno;
        }
        
        size_t total_size = offset + size;
        unsigned char *plaintext = NULL;
        
        // Jika file sudah ada dan offset > 0, baca file dulu
        if (st.st_size > 0 && offset > 0) {
            // Proses membaca file yang ada, dekripsi, dan siapkan buffer
            // ...
        } else {
            plaintext = malloc(total_size);
            if (!plaintext) {
                close(fd);
                return -ENOMEM;
            }
            memset(plaintext, 0, total_size);
        }
        
        // Salin data baru ke posisi offset
        memcpy(plaintext + offset, buf, size);
        
        // Enkripsi dan tulis kembali
        unsigned char *ciphertext = malloc(total_size + AES_BLOCK_SIZE + AES_BLOCK_SIZE);
        if (!ciphertext) {
            free(plaintext);
            close(fd);
            return -ENOMEM;
        }
        
        int ciphertext_len = 0;
        if (!heaven_encrypt(plaintext, total_size, ciphertext, &ciphertext_len)) {
            free(plaintext);
            free(ciphertext);
            close(fd);
            return -EIO;
        }
        
        free(plaintext);
        lseek(fd, 0, SEEK_SET);
        if (ftruncate(fd, 0) == -1) {
            free(ciphertext);
            close(fd);
            return -errno;
        }
        
        ssize_t bytes_written = write(fd, ciphertext, ciphertext_len);
        free(ciphertext);
        close(fd);
        if (bytes_written != ciphertext_len)
            return -EIO;
        return size;
    }
    
    // Area 7sref (redirect)
    if (is_7sref_path(path)) {
        char target_path[PATH_MAX];
        if (transform_7sref_path(path, target_path)) {
            return maimai_write(target_path, buf, size, offset, fi);
        } else {
            return -ENOENT;
        }
    }
    
    // Default path
    char fullpath[PATH_MAX];
    maimai_fullpath(fullpath, path);
    int fd = open(fullpath, O_WRONLY);
    if (fd == -1)
        return -errno;
    int res = pwrite(fd, buf, size, offset);
    if (res == -1)
        res = -errno;
    close(fd);
    return res;
}

```
### Operasi create
```c
static int maimai_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    // Area starter
    if (is_starter_path(path)) {
        char transformed_path[PATH_MAX];
        transform_starter_path(transformed_path, path);
        int fd = open(transformed_path, O_CREAT | O_EXCL | O_WRONLY, mode);
        if (fd == -1)
            return -errno;
        fi->fh = fd;
        return 0;
    }
    
    // Implementasi serupa untuk area-area lain...
    
    // Area heaven (dengan enkripsi AES)
    if (is_heaven_path(path)) {
        char transformed_path[PATH_MAX];
        transform_heaven_path(transformed_path, path);
        int fd = open(transformed_path, O_CREAT | O_WRONLY, mode);
        if (fd == -1)
            return -errno;
        close(fd);
        fi->fh = 0;
        return 0;
    }
    
    // Area skystreet (dengan kompresi)
    if (is_skystreet_path(path)) {
        char transformed_path[PATH_MAX];
        transform_skystreet_path(transformed_path, path);
        unsigned char empty[] = "";
        unsigned char *compressed_data = NULL;
        size_t compressed_len = 0;
        int ret = skystreet_compress(empty, 0, &compressed_data, &compressed_len);
        if (ret != Z_OK)
            return -EIO;
        int fd = open(transformed_path, O_CREAT | O_WRONLY, mode);
        if (fd == -1) {
            free(compressed_data);
            return -errno;
        }
        write(fd, compressed_data, compressed_len);
        close(fd);
        free(compressed_data);
        fi->fh = 0;
        return 0;
    }
    
    // Area 7sref (redirect)
    if (is_7sref_path(path)) {
        char target_path[PATH_MAX];
        if (transform_7sref_path(path, target_path)) {
            return maimai_create(target_path, mode, fi);
        } else {
            return -ENOENT;
        }
    }
    
    // Default path
    char fullpath[PATH_MAX];
    maimai_fullpath(fullpath, path);
    int fd = open(fullpath, O_CREAT | O_EXCL | O_WRONLY, mode);
    if (fd == -1)
        return -errno;
    fi->fh = fd;
    return 0;
}

```
### Operasi unlink
```c
static int maimai_unlink(const char *path) {
    // Area starter
    if (is_starter_path(path)) {
        char transformed_path[PATH_MAX];
        transform_starter_path(transformed_path, path);
        int res = unlink(transformed_path);
        if (res == -1)
            return -errno;
        return 0;
    }
    
    // Implementasi serupa untuk area-area lain...
    
    // Area 7sref (redirect)
    if (is_7sref_path(path)) {
        char target_path[PATH_MAX];
        if (transform_7sref_path(path, target_path)) {
            return maimai_unlink(target_path);
        } else {
            return -ENOENT;
        }
    }
    
    // Default path
    char fullpath[PATH_MAX];
    maimai_fullpath(fullpath, path);
    int res = unlink(fullpath);
    if (res == -1)
        return -errno;
    return 0;
}

```
### Starter Chiho
Pada Starter Chiho, semua file yang masuk ke /fuse_dir/starter/ akan disimpan di backend /chiho/starter/ dengan tambahan ekstensi .mai
```c
static int is_starter_path(const char *path) {
    return (strncmp(path, "/fuse_dir/starter/", 18) == 0);
}

```
Transformasi nama file dilakukan agar file di backend memiliki ekstensi .mai:
```c
static void transform_starter_path(char *result, const char *path) {
    char filename[PATH_MAX];
    strcpy(filename, path + 18);
    sprintf(result, "%s/chiho/starter/%s.mai", maimai_root_dir, filename);
}

```
Pada operasi read dan write, file diakses secara normal tanpa encoding atau enkripsi, hanya transformasi nama file saja.
### Metropolis (Metro) Chiho
Di Metropolis Chiho, file di /fuse_dir/metro/ disimpan di backend /chiho/metro/ dengan ekstensi .ccc. Isi file di-backend dienkripsi dengan shift karakter berdasarkan posisi byte, dan didekripsi saat dibaca di FUSE. 
```c
static int is_metro_path(const char *path) {
    return (strncmp(path, "/fuse_dir/metro/", 16) == 0);
}
static void transform_metro_path(char *result, const char *path) {
    char filename[PATH_MAX];
    strcpy(filename, path + 16);
    sprintf(result, "%s/chiho/metro/%s.ccc", maimai_root_dir, filename);
}

```
Fungsi enkripsi dan dekripsi karakter:
```c
static void metro_encrypt(const char *input, char *output, size_t size) {
    for (size_t i = 0; i < size; i++) {
        output[i] = (input[i] + (i % 256)) % 256;
    }
}
static void metro_decrypt(const char *input, char *output, size_t size) {
    for (size_t i = 0; i < size; i++) {
        output[i] = (input[i] - (i % 256) + 256) % 256;
    }
}

```
### Dragon Chiho
Pada Dragon Chiho, file di /fuse_dir/dragon/ disimpan di /chiho/dragon/ dengan ekstensi .rot dan di-backend dienkripsi menggunakan ROT13.
```c
static int is_dragon_path(const char *path) {
    return (strncmp(path, "/fuse_dir/dragon/", 17) == 0);
}
static void transform_dragon_path(char *result, const char *path) {
    char filename[PATH_MAX];
    strcpy(filename, path + 17);
    sprintf(result, "%s/chiho/dragon/%s.rot", maimai_root_dir, filename);
}

```
fungsi ROT13:
```c
static void rot13(const char *input, char *output, size_t size) {
    for (size_t i = 0; i < size; i++) {
        char c = input[i];
        if (isalpha(c)) {
            if ((c >= 'a' && c <= 'm') || (c >= 'A' && c <= 'M'))
                c += 13;
            else
                c -= 13;
        }
        output[i] = c;
    }
}

```
### Blackrose Chiho
Di Blackrose Chiho, file di /fuse_dir/blackrose/ disimpan di /chiho/blackrose/ dengan ekstensi .bin tanpa encoding atau enkripsi tambahan.
```c
static int is_blackrose_path(const char *path) {
    return (strncmp(path, "/fuse_dir/blackrose/", 20) == 0);
}
static void transform_blackrose_path(char *result, const char *path) {
    char filename[PATH_MAX];
    strcpy(filename, path + 20);
    sprintf(result, "%s/chiho/blackrose/%s.bin", maimai_root_dir, filename);
}

```
### HeaveN Chiho
Heaven Chiho menggunakan enkripsi AES-256-CBC dengan IV random untuk setiap file di /chiho/heaven/ (ekstensi .enc). Path handler:
```c
static int is_heaven_path(const char *path) {
    return (strncmp(path, "/fuse_dir/heaven/", 17) == 0);
}
static void transform_heaven_path(char *result, const char *path) {
    char filename[PATH_MAX];
    strcpy(filename, path + 17);
    sprintf(result, "%s/chiho/heaven/%s.enc", maimai_root_dir, filename);
}

```
Enkripsi dan dekripsi menggunakan OpenSSL:
```c
static int heaven_encrypt(const unsigned char *plaintext, int plaintext_len,
                         unsigned char *ciphertext, int *ciphertext_len) {
    EVP_CIPHER_CTX *ctx;
    int len, ciphertext_length;
    unsigned char iv[AES_BLOCK_SIZE];
    RAND_bytes(iv, AES_BLOCK_SIZE); // IV random
    memcpy(ciphertext, iv, AES_BLOCK_SIZE);
    ciphertext_length = AES_BLOCK_SIZE;
    ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, heaven_key, iv);
    EVP_EncryptUpdate(ctx, ciphertext + ciphertext_length, &len, plaintext, plaintext_len);
    ciphertext_length += len;
    EVP_EncryptFinal_ex(ctx, ciphertext + ciphertext_length, &len);
    ciphertext_length += len;
    EVP_CIPHER_CTX_free(ctx);
    *ciphertext_len = ciphertext_length;
    return 1;
}
static int heaven_decrypt(const unsigned char *ciphertext, int ciphertext_len,
                         unsigned char *plaintext, int *plaintext_len) {
    EVP_CIPHER_CTX *ctx;
    int len, plaintext_length;
    unsigned char iv[AES_BLOCK_SIZE];
    memcpy(iv, ciphertext, AES_BLOCK_SIZE);
    ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, heaven_key, iv);
    EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext + AES_BLOCK_SIZE, ciphertext_len - AES_BLOCK_SIZE);
    plaintext_length = len;
    EVP_DecryptFinal_ex(ctx, plaintext + len, &len);
    plaintext_length += len;
    EVP_CIPHER_CTX_free(ctx);
    *plaintext_len = plaintext_length;
    return 1;
}

```
### Skystreet Chiho
```c
static int is_skystreet_path(const char *path) {
    return (strncmp(path, "/fuse_dir/skystreet/", 20) == 0);
}
static void transform_skystreet_path(char *result, const char *path) {
    char filename[PATH_MAX];
    strcpy(filename, path + 20);
    sprintf(result, "%s/chiho/skystreet/%s.gz", maimai_root_dir, filename);
}

```
Fungsi kompresi dan dekompresi:
```c
static int skystreet_compress(const unsigned char *data, size_t data_len,
                             unsigned char **compressed_data, size_t *compressed_len) {
    // ... kode zlib deflate seperti pada kode sumber
}
static int skystreet_decompress(const unsigned char *compressed_data, size_t compressed_len,
                               unsigned char **data, size_t *data_len) {
    // ... kode zlib inflate seperti pada kode sumber
}

```
### 7sRef Chiho
7sRef Chiho adalah gateway ke semua area lain. File di /fuse_dir/7sref/ memiliki format nama area_namafile dan otomatis di-redirect ke area yang sesuai. 
```c
static int is_7sref_path(const char *path) {
    return (strncmp(path, "/fuse_dir/7sref/", 16) == 0);
}
static int extract_7sref_info(const char *path, char *area, char *filename) {
    const char *file_part = path + 16;
    const char *underscore = strchr(file_part, '_');
    if (!underscore) return 0;
    size_t area_len = underscore - file_part;
    strncpy(area, file_part, area_len);
    area[area_len] = '\0';
    strcpy(filename, underscore + 1);
    return 1;
}
static int transform_7sref_path(const char *path, char *target_path) {
    char area[PATH_MAX];
    char filename[PATH_MAX];
    if (!extract_7sref_info(path, area, filename)) return 0;
    sprintf(target_path, "/fuse_dir/%s/%s", area, filename);
    return 1;
}

```
pada area ini akan memanggil handler area target yang sesuai berdasarkan nama file.

### Revisi
- Kesalahan Struktur
- Area Heaven tidak bisa baca file.txt.gz
### Kesimpulan
Setiap area memiliki fungsi deteksi path (is_*_path), transformasi path (transform_*_path), dan perlakuan data (enkripsi, kompresi, dsb) yang diimplementasikan dalam fungsi-fungsi khusus. Handler ini bekerja secara transparan sehingga pengguna tetap melihat dan mengakses file secara normal di FUSE

