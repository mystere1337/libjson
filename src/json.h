#ifndef LIBJSON_JSON_H
#define LIBJSON_JSON_H

typedef struct json_container_s json_container_t;

json_container_t* json_from_file(const char* path);
json_container_t* json_from_string(const char* str);

void json_free(json_container_t* container);
int json_save(json_container_t* container, const char* path);

#endif //LIBJSON_JSON_H
