#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>

#define IMAGE_DIR "image"
#define TEXT_DIR "texts"
#define LOG_FILE "conversion.log"
#define BUFFER_SIZE 65536
#define ZIP_URL "https://drive.usercontent.google.com/u/0/uc?id=1hi_GDdP51Kn2JJMw02WmCOxuc3qrXzh5&export=download"
#define ZIP_FILE "anomali_texts.zip"

// Membuat folder jika belum ada
void create_directory(const char *dirname) {
    struct stat st = {0};
    if (stat(dirname, &st) == -1) {
        mkdir(dirname, 0700);
    }
}

// Mendapatkan waktu saat ini dalam format YYYY-mm-dd dan HH:MM:SS
void get_current_time(char *date_out, char *time_out) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(date_out, 20, "%Y-%m-%d", t);
    strftime(time_out, 20, "%H:%M:%S", t);
}

// Mengunduh file ZIP dari internet
void download_zip() {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "wget -O %s \"%s\"", ZIP_FILE, ZIP_URL);
    printf("🔽 Mengunduh ZIP dari Google Drive...\n");
    int result = system(cmd);
    if (result != 0) {
        fprintf(stderr, "❌ Gagal mengunduh file ZIP.\n");
        exit(1);
    }
}

// Mengekstrak file ZIP ke folder texts/
void unzip_and_cleanup() {
    char unzip_cmd[256];
    snprintf(unzip_cmd, sizeof(unzip_cmd), "unzip -o %s -d %s", ZIP_FILE, TEXT_DIR);
    printf("📦 Mengekstrak ZIP ke folder '%s'...\n", TEXT_DIR);
    int result = system(unzip_cmd);
    if (result != 0) {
        fprintf(stderr, "❌ Gagal mengekstrak ZIP.\n");
        exit(1);
    }

    // Hapus file zip
    if (remove(ZIP_FILE) == 0) {
        printf("🧹 ZIP dihapus: %s\n", ZIP_FILE);
    } else {
        perror("Gagal menghapus file ZIP");
    }
}

// Konversi string hex ke file gambar
void convert_hex_to_image(const char *filepath, const char *filename) {
    FILE *f = fopen(filepath, "r");
    if (!f) {
        perror("Gagal membuka file");
        return;
    }

    char *hex_data = malloc(BUFFER_SIZE);
    if (!hex_data) {
        fclose(f);
        fprintf(stderr, "Gagal mengalokasi memori\n");
        return;
    }

    fscanf(f, "%s", hex_data);
    fclose(f);

    char date[20], time_str[20];
    get_current_time(date, time_str);

    // Potong nama file tanpa .txt
    char base_name[256] = {0};
    strncpy(base_name, filename, strlen(filename) - 4);
    base_name[strlen(filename) - 4] = '\0';

    char image_filename[512];
    snprintf(image_filename, sizeof(image_filename),
             "%s/%s_image_%s_%s.png", IMAGE_DIR, base_name, date, time_str);

    FILE *img = fopen(image_filename, "wb");
    if (!img) {
        perror("Gagal membuat file gambar");
        free(hex_data);
        return;
    }

    size_t hex_len = strlen(hex_data);
    if (hex_len % 2 != 0) {
        fprintf(stderr, "Panjang hex tidak genap untuk file %s, dilewati.\n", filename);
        fclose(img);
        free(hex_data);
        return;
    }

    for (size_t i = 0; i < hex_len; i += 2) {
        unsigned int byte;
        if (sscanf(&hex_data[i], "%2x", &byte) != 1) {
            fprintf(stderr, "Gagal parsing hex di posisi %zu pada %s\n", i, filename);
            fclose(img);
            free(hex_data);
            return;
        }
        fputc(byte, img);
    }

    fclose(img);
    free(hex_data);

    // Tulis log dengan format sesuai permintaan
    FILE *log = fopen(LOG_FILE, "a");
    if (log) {
        char image_file_only[512];  // buffer aman
        snprintf(image_file_only, sizeof(image_file_only),
                 "%s_image_%s_%s.png", base_name, date, time_str);

        fprintf(log, "[%s][%s]: Successfully converted hexadecimal text %s to %s\n",
                date, time_str, filename, image_file_only);
        fclose(log);
    } else {
        fprintf(stderr, "❌ Gagal membuka %s\n", LOG_FILE);
    }
}

int main() {
    // Siapkan folder
    create_directory(IMAGE_DIR);
    create_directory(TEXT_DIR);

    // Unduh dan ekstrak
    download_zip();
    unzip_and_cleanup();

    // Proses semua file .txt dalam folder texts
    DIR *dir;
    struct dirent *entry;
    dir = opendir(TEXT_DIR);
    if (!dir) {
        perror("Gagal membuka folder 'texts'");
        return 1;
    }

    char filepath[512];
    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, ".txt")) {
            snprintf(filepath, sizeof(filepath), "%s/%s", TEXT_DIR, entry->d_name);
            convert_hex_to_image(filepath, entry->d_name);
        }
    }

    closedir(dir);
    printf("✅ Semua file berhasil diproses. Cek folder 'image/' dan file 'conversion.log'.\n");
    return 0;
}