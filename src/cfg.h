#ifndef LIBCFG_CFG_H
#define LIBCFG_CFG_H

#include <stdarg.h>
#include <glob.h>
#include <stdbool.h>

enum setting_type_e {
    Bool,
    Char,
    Short,
    Int,
    Long,
    Float,
    Double,
    String,
};

struct setting_t {
    char* name;
    enum setting_type_e type;

    union {
        bool bool_type;
        char char_type;
        short short_type;
        int int_type;
        long long_type;
        float float_type;
        double double_type;
        char* string_type;
    };
};

struct cfg_t {
    int fd;

    struct setting_t* settings;
    size_t settings_count;
};

struct setting_t create_bool_setting(char* name, bool value);
struct setting_t create_char_setting(char* name, char value);
struct setting_t create_short_setting(char* name, short value);
struct setting_t create_int_setting(char* name, int value);
struct setting_t create_long_setting(char* name, long value);
struct setting_t create_float_setting(char* name, float value);
struct setting_t create_double_setting(char* name, double value);
struct setting_t create_string_setting(char* name, char* value);

struct cfg_t* load_config(char *path, struct setting_t* settings, size_t settings_count);

#endif //LIBCFG_CFG_H
