#ifndef LIBJSON_JSON_H
#define LIBJSON_JSON_H

#include <stddef.h>

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

char* json_get_string(obj_t* obj, const char* str, char separator);
int json_get_bool(obj_t* obj, const char* str, char separator);
long long json_get_integer(obj_t* obj, const char* str, char separator);
obj_t* json_get_object(obj_t* obj, const char* str, char separator);
long double json_get_floating(obj_t* obj, const char* str, char separator);

int json_set_string(obj_t* obj, const char* key, char separator, const char* value);
int json_set_bool(obj_t* obj, const char* key, char separator, int value);
int json_set_integer(obj_t* obj, const char* key, char separator, long long value);
int json_set_floating(obj_t* obj, const char* key, char separator, long double value);
int json_set_object(obj_t* obj, const char* key, char separator, obj_t* value);

int json_remove_setting(obj_t* obj, const char* key, char separator);

void json_free(obj_t* obj);
int json_save(obj_t* obj, const char* path);

char* json_dump(obj_t* obj, int format);
void json_print(obj_t* obj, int format);

#endif //LIBJSON_JSON_H
