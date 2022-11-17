#ifndef LIBJSON_JSON_H
#define LIBJSON_JSON_H

#include <stddef.h>
#include <sys/queue.h>

enum json_value_type_e {
    JSON_VALUE_BOOL,
    JSON_VALUE_INT,
    JSON_VALUE_FLOAT,
    JSON_VALUE_STR,
    JSON_VALUE_OBJ,
    JSON_VALUE_ARR,
    JSON_VALUE_NULL,
};

enum json_token_type_e {
    JSON_TOKEN_BOOL,
    JSON_TOKEN_NUMBER,
    JSON_TOKEN_SYNTAX,
    JSON_TOKEN_NULL,
    JSON_TOKEN_STRING,
};

enum json_container_type_e {
    JSON_CONTAINER_ARRAY,
    JSON_CONTAINER_OBJ,
};

typedef struct json_container_s json_container_t;
typedef struct json_obj_s json_obj_t;
typedef struct json_setting_s json_setting_t;
typedef struct json_array_s json_array_t;
typedef struct json_value_s json_value_t;
typedef struct json_string_s json_string_t;
typedef struct json_token_s json_token_t;

/**
 * Contains a JSON token for parsing
 */
struct json_token_s {
    enum json_token_type_e type;
    json_string_t* string;

    TAILQ_ENTRY(json_token_s) next;
};

/**
 * Contains a JSON container, either Object or Array
 */
struct json_container_s {
    enum json_container_type_e type;

    union {
        json_array_t* array;
        json_obj_t* obj;
    };
};

/**
 * Contains a json object ({"key1": true, "key2": false})
 */
struct json_obj_s {
    json_setting_t** settings;
    size_t settings_count;
};

/**
 * Contains a string without null terminator
 */
struct json_string_s {
    char* value;
    size_t len;
};

/**
 * Contains a json string (boolean, string, number, object or array)
 */
struct json_value_s {
    enum json_value_type_e type;

    union {
        int bool_type;
        long long long_type;
        long double double_type;
        json_string_t* string_type;
        json_obj_t* obj_type;
        json_array_t* array_type;
    };
};

/**
 * Contains an array of values ([4, true, false, {}, null, "hello"])
 */
struct json_array_s {
    json_value_t** values;
    size_t values_count;
};

/**
 * Contains a JSON setting ("key": values)
 */
struct json_setting_s {
    json_string_t* name;
    json_value_t* value;
};

json_array_t* json_parse_array(json_token_t* first);
json_obj_t* json_parse_object(json_token_t* first);

void json_free_array(json_array_t* array);
void json_free_value(json_value_t* value);
void json_free_setting(json_setting_t* setting);
void json_free_object(json_obj_t* obj);
void json_free_container(json_container_t* con);

json_container_t* json_from_file(const char *path);
json_container_t* json_from_string(const char* str);

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
int json_save(json_container_t* obj, const char* path);

void json_dump_string(json_string_t* str, int fd);
void json_dump_value(json_value_t* val, int format, int fd);
void json_dump_setting(json_setting_t* set, int format, int fd);
void json_dump_object(json_obj_t* obj, int format, int fd);
void json_dump_array(json_array_t* arr, int format, int fd);
void json_dump_container(json_container_t* con, int format, int fd);

#endif //LIBJSON_JSON_H
