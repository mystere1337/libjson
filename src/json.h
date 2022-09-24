#ifndef LIBCFG_CFG_H
#define LIBCFG_CFG_H

#include <stdarg.h>
#include <glob.h>
#include <stdbool.h>

enum setting_type_e {
    Boolean,
    Integer,
    Floating,
    String,
    Object,
};

typedef struct obj_s obj_t;
typedef struct setting_s setting_t;

struct obj_s {
    setting_t** settings;
    size_t settings_count;
};

struct setting_s {
    char* name;
    enum setting_type_e type;

    union {
        int bool_type;
        long long long_type;
        long double double_type;
        char* string_type;
        obj_t* obj_type;
    };
};

obj_t* json_from_file(const char *path);
obj_t* json_from_string(const char* str);
char* json_get_string(obj_t* obj, const char* str);
void json_free(obj_t* obj);
int json_save(obj_t* obj, const char* path);

#endif //LIBCFG_CFG_H
