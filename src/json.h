#ifndef LIBJSON_JSON_H
#define LIBJSON_JSON_H

#include <stddef.h>

enum json_setting_type_e {
    Boolean,
    Integer,
    Floating,
    String,
    Object,
};

typedef struct json_obj_s json_obj_t;
typedef struct json_setting_s json_setting_t;

struct json_obj_s {
    json_setting_t** settings;
    size_t settings_count;
};

struct json_setting_s {
    char* name;
    enum json_setting_type_e type;

    union {
        int bool_type;
        long long long_type;
        long double double_type;
        char* string_type;
        json_obj_t* obj_type;
    };
};

json_obj_t* json_from_file(const char *path);
json_obj_t* json_from_string(const char* str);

char* json_get_string(json_obj_t* obj, const char* str, char separator);
int json_get_bool(json_obj_t* obj, const char* str, char separator);
long long json_get_integer(json_obj_t* obj, const char* str, char separator);
json_obj_t* json_get_object(json_obj_t* obj, const char* str, char separator);
long double json_get_floating(json_obj_t* obj, const char* str, char separator);

int json_set_string(json_obj_t* obj, const char* key, char separator, const char* value);
int json_set_bool(json_obj_t* obj, const char* key, char separator, int value);
int json_set_integer(json_obj_t* obj, const char* key, char separator, long long value);
int json_set_floating(json_obj_t* obj, const char* key, char separator, long double value);
int json_set_object(json_obj_t* obj, const char* key, char separator, json_obj_t* value);

int json_remove_setting(json_obj_t* obj, const char* key, char separator);

void json_free(json_obj_t* obj);
int json_save(json_obj_t* obj, const char* path);

char* json_dump(json_obj_t* obj, int format);
void json_print(json_obj_t* obj, int format);

#endif //LIBJSON_JSON_H
