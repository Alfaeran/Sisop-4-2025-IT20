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

#define PART_SIZE 1024
#define PART_COUNT 14
#define MAX_FILE_SIZE (1024 * 1024 * 100)
#define FILENAME "Baymax.jpeg"

char base_dir[1024];
char activity_log_path[1024];

static char *write_buffer = NULL;
static size_t write_size = 0;
static char last_read_file[256] = "";
static time_t last_read_time = 0;

void get_fragment_path(char *dest, const char *filename, int index) {
snprintf(dest, 1024, "%s/relics/%s.%03d", base_dir, filename, index);
}

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

static int baymax_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
off_t offset, struct fuse_file_info *fi,
enum fuse_readdir_flags flags) {
if (strcmp(path, "/") != 0) return -ENOENT;
filler(buf, ".", NULL, 0, 0);
filler(buf, "..", NULL, 0, 0);
filler(buf, FILENAME, NULL, 0, 0);
return 0;
}

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

static int baymax_open(const char *path, struct fuse_file_info *fi) {
if (strcmp(path + 1, FILENAME) != 0) return -ENOENT;
return 0;
}

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

static int baymax_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
if (write_buffer) free(write_buffer);
write_buffer = malloc(MAX_FILE_SIZE);
if (!write_buffer) return -ENOMEM;
write_size = 0;
return 0;
}

static int baymax_write(const char *path, const char *buf, size_t size, off_t offset,
struct fuse_file_info *fi) {
if (!write_buffer) return -EIO;
if (offset + size > MAX_FILE_SIZE) return -EFBIG;
memcpy(write_buffer + offset, buf, size);
if (offset + size > write_size) write_size = offset + size;
return size;
}

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

int main(int argc, char *argv[]) {
getcwd(base_dir, sizeof(base_dir));
snprintf(activity_log_path, sizeof(activity_log_path), "%s/activity.log", base_dir);
umask(0);
return fuse_main(argc, argv, &baymax_oper, NULL);
}

