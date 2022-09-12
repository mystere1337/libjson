#include "cfg.h"

#include <fcntl.h>
#include <malloc.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>

size_t get_file_size(int fd) {
    struct stat s;

    int status = fstat(fd, &s);
    return s.st_size;
}

struct setting_t create_bool_setting(char* name, bool value) {
    struct setting_t setting;

    setting.name = name;
    setting.bool_type = value;
    setting.type = Bool;
    return setting;
}

struct setting_t create_char_setting(char* name, char value) {
    struct setting_t setting;

    setting.name = name;
    setting.char_type = value;
    setting.type = Char;
    return setting;
}

struct setting_t create_short_setting(char *name, short value) {
    struct setting_t setting;

    setting.name = name;
    setting.short_type = value;
    setting.type = Short;
    return setting;
}

struct setting_t create_int_setting(char* name, int value) {
    struct setting_t setting;

    setting.name = name;
    setting.int_type = value;
    setting.type = Int;
    return setting;
}

struct setting_t create_long_setting(char* name, long value) {
    struct setting_t setting;

    setting.name = name;
    setting.long_type = value;
    setting.type = Long;
    return setting;
}

struct setting_t create_float_setting(char* name, float value) {
    struct setting_t setting;

    setting.name = name;
    setting.float_type = value;
    setting.type = Float;
    return setting;
}

struct setting_t create_double_setting(char* name, double value) {
    struct setting_t setting;

    setting.name = name;
    setting.double_type = value;
    setting.type = Double;
    return setting;
}

struct setting_t create_string_setting(char* name, char* value) {
    struct setting_t setting;

    setting.name = name;
    setting.string_type = value;
    setting.type = String;
    return setting;
}

void write_int(int fd, struct setting_t* setting) {
    size_t needed = snprintf(NULL, 0, "%s=%d\n", setting->name, setting->int_type) + 1;
    char *line = malloc(needed);

    snprintf(line, needed, "%s=%d\n", setting->name, setting->int_type);
    write(fd, line, needed);
}

void write_float(int fd, struct setting_t* setting) {
    size_t needed = snprintf(NULL, 0, "%s=%f\n", setting->name, setting->float_type) + 1;
    char *line = malloc(needed);

    snprintf(line, needed, "%s=%f\n", setting->name, setting->float_type);
    write(fd, line, needed);
}

void write_double(int fd, struct setting_t* setting) {
    size_t needed = snprintf(NULL, 0, "%s=%f\n", setting->name, setting->double_type) + 1;
    char *line = malloc(needed);

    snprintf(line, needed, "%s=%f\n", setting->name, setting->double_type);
    write(fd, line, needed);
}

void write_bool(int fd, struct setting_t* setting) {
    size_t needed = snprintf(NULL, 0, "%s=%s\n", setting->name, setting->bool_type ? "true" : "false") + 1;
    char *line = malloc(needed);

    snprintf(line, needed, "%s=%s\n", setting->name, setting->bool_type ? "true" : "false");
    write(fd, line, needed);
}

void write_long(int fd, struct setting_t* setting) {
    size_t needed = snprintf(NULL, 0, "%s=%ld\n", setting->name, setting->long_type) + 1;
    char *line = malloc(needed);

    snprintf(line, needed, "%s=%ld\n", setting->name, setting->long_type);
    write(fd, line, needed);
}

void write_string(int fd, struct setting_t* setting) {
    size_t needed = snprintf(NULL, 0, "%s=%s\n", setting->name, setting->string_type) + 1;
    char *line = malloc(needed);

    snprintf(line, needed, "%s=%s\n", setting->name, setting->string_type);
    write(fd, line, needed);
}

void write_char(int fd, struct setting_t* setting) {
    size_t needed = snprintf(NULL, 0, "%s=%c\n", setting->name, setting->char_type) + 1;
    char *line = malloc(needed);

    snprintf(line, needed, "%s=%c\n", setting->name, setting->char_type);
    write(fd, line, needed);
}

void write_short(int fd, struct setting_t* setting) {
    size_t needed = snprintf(NULL, 0, "%s=%d\n", setting->name, setting->short_type) + 1;
    char *line = malloc(needed);

    snprintf(line, needed, "%s=%d\n", setting->name, setting->short_type);
    write(fd, line, needed);
}

void write_config(struct cfg_t* cfg) {
    ftruncate(cfg->fd, 0);

    for (size_t i = 0; i < cfg->settings_count; i++) {
        switch (cfg->settings[i].type) {
            case Int: write_int(cfg->fd, &cfg->settings[i]);
                continue;
            case Float: write_float(cfg->fd, &cfg->settings[i]);
                continue;
            case Double: write_double(cfg->fd, &cfg->settings[i]);
                continue;
            case Bool: write_bool(cfg->fd, &cfg->settings[i]);
                continue;
            case Long: write_long(cfg->fd, &cfg->settings[i]);
                continue;
            case String: write_string(cfg->fd, &cfg->settings[i]);
                continue;
            case Char: write_char(cfg->fd, &cfg->settings[i]);
                continue;
            case Short: write_short(cfg->fd, &cfg->settings[i]);
                continue;
            default:
                break;
        };
    }
}

struct cfg_t* load_config(char *path, struct setting_t* settings, size_t settings_count) {
    struct cfg_t* cfg = malloc(sizeof(struct cfg_t));

    cfg->settings = settings;
    cfg->settings_count = settings_count;
    cfg->fd = open(path, O_RDWR | O_CREAT | O_APPEND, 0600);

    if (get_file_size(cfg->fd) == 0) {
        write_config(cfg);
    }

    close(cfg->fd);
}
