#include "json.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <ctype.h>

TAILQ_HEAD(json_tailhead_s, json_token_s) json_head;

/**
 * Initializes an object to be populated
 * @return Empty object
 */
json_obj_t* json_init_obj() {
    json_obj_t* obj = malloc(sizeof(json_obj_t));

    obj->settings = malloc(0);
    obj->settings_count = 0;
    return obj;
}

/**
 * Initializes an array to be populated
 * @return Empty array
 */
json_array_t* json_init_array() {
    json_array_t* arr = malloc(sizeof(json_array_t));

    arr->values = malloc(0);
    arr->values_count = 0;
    return arr;
}

/**
 * Creates a json string, check length to avoid unexpected behavior
 * @param src Source to copy string from
 * @param len Length of the string to copy
 * @return
 */
json_string_t* json_create_string(const char* src, size_t len) {
    json_string_t* str = malloc(sizeof(json_string_t));

    str->len = len;
    str->value = malloc(len);

    if (str->value == NULL) {
        return NULL;
    }

    strncpy(str->value, src, len);

    return str;
}

/**
 * Checks if current character is whitespace
 * @param c Character to check
 * @return 1 if current character is whitespace, 0 otherwise
 */
int json_symbol_is_whitespace(const char c) {
    return c == ' ' || c == '\t' || c == '\v' || c == '\r' || c == '\n' || c == '\f';
}

/**
 * Checks if current character is a json syntax character
 * @param c Character to check
 * @return 1 if current character is part of JSON syntax, 0 otherwise
 */
int json_symbol_is_syntax(const char c) {
    return c == ':' || c == ',' || c == '[' || c == ']' || c == '{' || c == '}';
}

/**
 * Check if symbol is string
 * @param str Lexer cursor
 * @return 1 if symbol is string, 0 otherwise
 */
int json_symbol_is_string(const char* str) {
    if (str[0] != '"') {
        return 0;
    }

    for (size_t pos = 1; str[pos] != '\0'; pos++) {
        if (str[pos] == '"' && str[pos - 1] != '\\') {
            return 1;
        }
    }

    return 0;
}

/**
 * Check if symbol is 'null'
 * @param str Lexer cursor
 * @return 1 if symbol is null, 0 otherwise
 */
int json_symbol_is_null(const char* str) {
    return strlen(str) >= 4 && strncmp(str, "null", 4) == 0;
}

/**
 * Check if symbol is a number
 * @param str Lexer cursor
 * @return 1 if symbol is a number, 0 otherwise
 */
int json_symbol_is_number(const char* str) {
    char prev = '\0';
    int exponent = 0;
    int dot = 0;

    for (size_t pos = 0; str[pos] != '\0'; pos++) {
        if (isdigit(str[pos])) {
            prev = str[pos];
            continue;
        }
        if (str[pos] == '+' && prev == 'e') {
            prev = '+';
            continue;
        }
        if (str[pos] == '-' && (pos == 0 || prev == 'e')) {
            prev = '-';
            continue;
        }
        if ((str[pos] == 'e' || str[pos] == 'E') && isdigit(prev) && exponent == 0) {
            prev = 'e';
            exponent = 1; // There can only be one exponent
            dot = 1; // Set dot to 1 since there can't be any dot after the exponent
            continue;
        }
        if (str[pos] == '.' && isdigit(prev) && dot == 0) {
            prev = '.';
            dot = 1; // There can only be one dot
            continue;
        }
        if (json_symbol_is_whitespace(str[pos]) && pos != 0) {
            continue;
        }
        if (json_symbol_is_syntax(str[pos])) {
            if (!isdigit(prev)) {
                return 0;
            }

            return 1;
        }

        return 0;
    }

    return 0;
}

/**
 * Check if symbol is bool
 * @param str Lexer cursor
 * @return 1 if symbol is a bool, 0 otherwise
 */
int json_symbol_is_bool(const char* str) {
    return strlen(str) >= 4 && strncmp(str, "true", 4) == 0 || strncmp(str, "false", 5) == 0;
}

/**
 * Tokenize symbol at position, must check if symbol is string before
 * @param str Lexer cursor
 * @return Tokenized string symbol
 */
json_token_t* json_lex_string(const char* str) {
    json_token_t* token = malloc(sizeof(json_token_t));

    for (size_t pos = 1; str[pos] != '\0'; pos++) {
        if (str[pos] == '"' && str[pos - 1] != '\\') {
            token->string = json_create_string(&str[1], pos - 1);
            token->type = JSON_TOKEN_STRING;
            return token;
        }
    }

    return token;
}

/**
 * Tokenize symbol at position, must check if symbol is JSON syntax symbol before
 * @param str Lexer cursor
 * @return Tokenized syntax symbol
 */
json_token_t* json_lex_syntax(const char* str) {
    json_token_t* token = malloc(sizeof(json_token_t));

    token->string = json_create_string(str, 1);
    token->type = JSON_TOKEN_SYNTAX;
    return token;
}

/**
 * Tokenize symbol at position, must check if symbol is number before using json_symbol_is_number
 * @param str Lexer cursor
 * @return Tokenized number symbol
 */
json_token_t* json_lex_number(const char* str) {
    json_token_t* token = malloc(sizeof(json_token_t));

    for (size_t pos = 0; str[pos] != '\0'; pos++) {
        if (json_symbol_is_syntax(str[pos]) || json_symbol_is_whitespace(str[pos])) {
            token->string = json_create_string(str, pos);
            token->type = JSON_TOKEN_NUMBER;
            return token;
        }
    }

    return token;
}

/**
 * Tokenize symbol at position, must check if symbol is boolean before
 * @param str Lexer cursor
 * @return Tokenized bool symbol
 */
json_token_t* json_lex_bool(const char* str) {
    json_token_t* token = malloc(sizeof(json_token_t));

    token->string = json_create_string(str, strncmp(str, "true", 4) == 0 ? 4 : 5);
    token->type = JSON_TOKEN_BOOL;
    return token;
}

/**
 * Tokenize symbol at position, must check if symbol is null     before
 * @param str Lexer cursor
 * @return Tokenized null symbol
 */
json_token_t* json_lex_null(const char* str) {
    json_token_t* token = malloc(sizeof(json_token_t));

    token->string = json_create_string(str, 4);
    token->type = JSON_TOKEN_NULL;
    return token;
}

/**
 * Frees a JSON string type
 * @param string The string to be freed
 */
void json_free_string(json_string_t* string) {
    free(string->value);
    free(string);
}

/**
 * Frees a lexer token
 * @param token JSON lexer token
 */
void json_free_token(json_token_t* token) {
    json_free_string(token->string);
    free(token);
}

/**
 * Frees an JSON array
 * @param array JSON array
 */
void json_free_array(json_array_t* array) {
    for (size_t i = 0; i < array->values_count; i++) {
        json_free_value(array->values[i]);
    }

    free(array->values);
    free(array);
}

/**
 * Frees a JSON string
 * @param value JSON string
 */
void json_free_value(json_value_t* value) {
    if (value->type == JSON_VALUE_STR) {
        free(value->string_type);
    }
    if (value->type == JSON_VALUE_OBJ) {
        json_free_object(value->obj_type);
    }
    if (value->type == JSON_VALUE_ARR) {
        json_free_array(value->array_type);
    }

    free(value);
}

/**
 * Frees a JSON setting
 * @param setting JSON setting
 */
void json_free_setting(json_setting_t* setting) {
    json_free_string(setting->name);
    json_free_value(setting->value);
    free(setting);
}

/**
 * Frees a JSON object
 * @param obj JSON object
 */
void json_free_object(json_obj_t* obj) {
    for (size_t i = 0; i < obj->settings_count; i++) {
        json_free_setting(obj->settings[i]);
    }

    free(obj->settings);
    free(obj);
}

/**
 * Frees a JSON container.
 * @param obj object to free
 */
void json_free_container(json_container_t* con) {
    if (con->type == JSON_CONTAINER_ARRAY) {
        json_free_array(con->array);
    } else {
        json_free_object(con->obj);
    }
    free(con);
}

/**
 * Adds the provided setting to the provided object
 * @param obj JSON object to add setting to
 * @param setting JSON setting to add to the object
 * @return 1 on success, 0 on failure
 */
int json_add_obj_setting(json_obj_t* obj, json_setting_t* setting) {
    json_setting_t** tmp = realloc(obj->settings, sizeof(json_setting_t*) * (obj->settings_count + 1));

    if (!tmp) {
        // error: something went wrong trying to reallocate
        return 0;
    }

    obj->settings_count += 1;
    obj->settings = tmp;

    obj->settings[obj->settings_count - 1] = setting;

    return 1;
}

/**
 * Clears the list of lexed tokens
 */
void json_lexer_clear_list() {
    json_token_t* current;

    while ((current = TAILQ_FIRST(&json_head))) {
        TAILQ_REMOVE(&json_head, current, next);
        json_free_token(current);
    }
}

/**
 * Removes the first object of the Lexer queue
 */
void json_lexer_remove_first() {
    json_token_t* first = TAILQ_FIRST(&json_head);

    TAILQ_REMOVE(&json_head, first, next);
    json_free_token(first);
}

json_value_t* json_parse_value(json_token_t* token) {
    json_value_t* value = malloc(sizeof(json_value_t));

    if (token->type == JSON_TOKEN_STRING) {
        value->type = JSON_VALUE_STR;
        value->string_type = json_create_string(token->string->value, token->string->len);
    } else if (token->type == JSON_TOKEN_BOOL) {
        value->type = JSON_VALUE_BOOL;
        value->bool_type = token->string->len == 4 ? 1 : 0;
    } else if (token->type == JSON_TOKEN_NULL) {
        value->type = JSON_VALUE_NULL;
    } else if (token->type == JSON_TOKEN_NUMBER) {
        char* null_terminated = malloc(token->string->len + 1);

        null_terminated[token->string->len] = '\0';
        strncpy(null_terminated, token->string->value, token->string->len);
        if (memchr(token->string->value, '.', token->string->len) != 0) {
            value->type = JSON_VALUE_FLOAT;
            value->double_type = strtod(null_terminated, NULL);
        } else {
            value->type = JSON_VALUE_INT;
            value->long_type = strtoll(null_terminated, NULL, 10);
        }

        free(null_terminated);
    } else if (token->type == JSON_TOKEN_SYNTAX) {
        if (strncmp(token->string->value, "[", 1) == 0) {
            value->type = JSON_VALUE_ARR;
            json_lexer_remove_first();
            json_array_t* arr = json_parse_array(TAILQ_FIRST(&json_head));

            if (arr == NULL) {
                // error: unexpected token
                json_free_value(value);
                return NULL;
            }

            value->array_type = arr;
        } else if (strncmp(token->string->value, "{", 1) == 0) {
            value->type = JSON_VALUE_OBJ;
            json_lexer_remove_first();
            json_obj_t * obj = json_parse_object(TAILQ_FIRST(&json_head));

            if (obj == NULL) {
                // error: unexpected token
                json_free_value(value);
                return NULL;
            }

            value->obj_type = obj;
        } else {
            // error: unexpected token
            json_free_value(value);
            return NULL;
        }
    }

    return value;
}

json_setting_t* json_parse_setting(json_token_t* first) {
    json_setting_t* setting = malloc(sizeof(json_setting_t));

    if (first->type != JSON_TOKEN_STRING) {
        // error: expected a string (setting key must be a string)
        json_free_setting(setting);
        return NULL;
    }

    setting->name = json_create_string(first->string->value, first->string->len);

    json_lexer_remove_first();
    first = TAILQ_FIRST(&json_head);

    if (first->type != JSON_TOKEN_SYNTAX || strncmp(first->string->value, ":", 1) != 0) {
        // error: expected a colon
        json_free_setting(setting);
        return NULL;
    }

    json_lexer_remove_first();
    first = TAILQ_FIRST(&json_head);

    json_value_t* value = json_parse_value(first);

    if (value == NULL) {
        // error: setting parsing didn't succeeded
        json_free_setting(setting);
        return NULL;
    }

    setting->value = value;

    json_lexer_remove_first();
    first = TAILQ_FIRST(&json_head);

    return setting;
}

json_obj_t* json_parse_object(json_token_t* first) {
    json_obj_t* obj = json_init_obj();

    if (first->type == JSON_TOKEN_SYNTAX) {
        if (strncmp(first->string->value, "}", 1) == 0) {
            return obj; // valid but empty
        }
    }

    for (; first != NULL; first = TAILQ_FIRST(&json_head)) {
        json_setting_t* setting = json_parse_setting(first);

        if (setting == NULL) {
            // error: expected a valid setting (got an unexpected token)
            json_free_object(obj);
            return NULL;
        }

        if (!json_add_obj_setting(obj, setting)) {
            // error: something went wrong trying to add setting to the object
            json_free_object(obj);
            return NULL;
        }

        first = TAILQ_FIRST(&json_head);

        if (first->type == JSON_TOKEN_SYNTAX && strncmp(first->string->value, "}", 1) == 0) {
            json_lexer_remove_first();
            return obj;
        } else if (first->type != JSON_TOKEN_SYNTAX || strncmp(first->string->value, ",", 1) != 0) {
            // error: expected a comma after a setting
            json_free_object(obj);
            return NULL;
        }

        json_lexer_remove_first();
    }

    // error: expected end of object bracket
    json_free_object(obj);
    return NULL;
}

json_array_t* json_parse_array(json_token_t* elem) {
    json_array_t* arr = json_init_array();

    if (elem->type == JSON_TOKEN_SYNTAX) {
        if (strncmp(elem->string->value, "]", 1) == 0) {
            return arr;
        }
    }

    return arr;
}

json_container_t* json_parse_container(json_token_t* elem) {
    json_container_t* container = malloc(sizeof(json_container_t));

    if (elem->type == JSON_TOKEN_SYNTAX) {
        if (strncmp(elem->string->value, "{", 1) == 0) {
            json_lexer_remove_first();
            container->type = JSON_CONTAINER_OBJ;
            container->obj = json_parse_object(TAILQ_FIRST(&json_head));
            return container->obj == NULL ? NULL : container;
        } else if (strncmp(elem->string->value, "[", 1) == 0) {
            json_lexer_remove_first();
            container->type = JSON_CONTAINER_ARRAY;
            container->array = json_parse_array(TAILQ_FIRST(&json_head));
            return container->array == NULL ? NULL : container;
        }
    }

    // error: invalid container (container must be either an object or an array)
    json_free_container(container);
    return NULL;
}

/**
 * Dumps a JSON string
 * @param str String to dump
 * @param fd File descriptor to dump in
 */
void json_dump_string(json_string_t* str, int fd) {
    dprintf(fd, "\"%.*s\"", (int)str->len, str->value);
}

/**
 * Dumps a JSON value
 * @param val Value to dump
 * @param fd File descriptor to dump in
 */
void json_dump_value(json_value_t* val, int format, int fd) {
    if (val->type == JSON_VALUE_STR) {
        json_dump_string(val->string_type, fd);
    } else if (val->type == JSON_VALUE_BOOL) {
        dprintf(fd, "%s", val->bool_type ? "true" : "false");
    } else if (val->type == JSON_VALUE_NULL) {
        dprintf(fd, "null");
    } else if (val->type == JSON_VALUE_FLOAT) {
        dprintf(fd, "%Lf", val->double_type);
    } else if (val->type == JSON_VALUE_INT) {
        dprintf(fd, "%lld", val->long_type);
    } else if (val->type == JSON_VALUE_OBJ) {
        json_dump_object(val->obj_type, format, fd);
    } else if (val->type == JSON_VALUE_ARR) {
        json_dump_array(val->array_type, format, fd);
    }
}

/**
 * Dumps a JSON setting
 * @param set Setting to dump
 * @param format 1 add formatting, 0 otherwise
 * @param fd File descriptor to dump in
 */
void json_dump_setting(json_setting_t* set, int format, int fd) {
    json_dump_string(set->name, fd);
    dprintf(fd, "%s", format ? ": " : ":");
    json_dump_value(set->value, format, fd);
}

/**
 * Dumps a JSON object
 * @param obj Object to dump
 * @param format 1 add formatting, 0 otherwise
 * @param fd File descriptor to dump in
 */
void json_dump_object(json_obj_t* obj, int format, int fd) {
    dprintf(fd, "%s", format ? "{\n" : "{");
    for (size_t i = 0; i < obj->settings_count; i++) {
        json_dump_setting(obj->settings[i], format, fd);
        if (i < obj->settings_count - 1) {
            dprintf(fd, "%s", format ? ",\n" : ",");
        }
    }
    dprintf(fd, "}");
}

/**
 * Dumps a JSON array
 * @param arr Array to dump
 * @param format 1 add formatting, 0 otherwise
 * @param fd File descriptor to dump in
 */
void json_dump_array(json_array_t* arr, int format, int fd) {
    dprintf(fd, "%s", format ? "[\n" : "[");
    for (size_t i = 0; i < arr->values_count; i++) {
        json_dump_value(arr->values[i], format, fd);
        if (i < arr->values_count - 1) {
            dprintf(fd, "%s", format ? ",\n" : ",");
        }
    }
    dprintf(fd, "]");
}

/**
 * Dumps a JSON container
 * @param container Container to be dumped
 * @param format Boolean value, 1 prints formatted, 0 otherwise
 * @param fd File descriptor to dump in
 */
void json_dump_container(json_container_t* container, int format, int fd) {
    if (container->type == JSON_CONTAINER_ARRAY) {
        json_dump_array(container->array, format, fd);
    } else {
        json_dump_object(container->obj, format, fd);
    }
}

/**
 * Converts a serialized JSON string to an object.
 * @param obj pointer to JSON object
 * @param str JSON string
 * @return -1 if unsuccessful, 0 if successful
 */
json_container_t* json_from_string(const char* str) {
    json_token_t* tok;
    TAILQ_INIT(&json_head);

    for (size_t pos = 0; str[pos] != '\0';) {
        if (json_symbol_is_string(&str[pos])) {
            tok = json_lex_string(&str[pos]);

            TAILQ_INSERT_TAIL(&json_head, tok, next);
            pos += tok->string->len + 2;
            continue;
        }
        if (json_symbol_is_number(&str[pos])) {
            tok = json_lex_number(&str[pos]);

            TAILQ_INSERT_TAIL(&json_head, tok, next);
            pos += tok->string->len;
            continue;
        }
        if (json_symbol_is_bool(&str[pos])) {
            tok = json_lex_bool(&str[pos]);

            TAILQ_INSERT_TAIL(&json_head, tok, next);
            pos += tok->string->len;
            continue;
        }
        if (json_symbol_is_null(&str[pos])) {
            tok = json_lex_null(&str[pos]);

            TAILQ_INSERT_TAIL(&json_head, tok, next);
            pos += 4;
            continue;
        }
        if (json_symbol_is_syntax(str[pos])) {
            tok = json_lex_syntax(&str[pos]);

            TAILQ_INSERT_TAIL(&json_head, tok, next);
            pos += 1;
            continue;
        }
        if (json_symbol_is_whitespace(str[pos])) {
            pos += 1;
            continue;
        }

        // error: unknown symbol
        json_lexer_clear_list();
        return NULL;
    }

    TAILQ_FOREACH(tok, &json_head, next) {
        printf("%.*s\n", tok->string->len, tok->string->value);
    }

    json_container_t* root = json_parse_container(TAILQ_FIRST(&json_head));

    json_lexer_clear_list();
    return root;
}

/**
 * Gets file size in bytes
 * @param fd File descriptor
 * @return Size of file in bytes
 */
size_t json_get_file_size(int fd) {
    struct stat s;

    fstat(fd, &s);
    return s.st_size;
}

/**
 * Gets content of file into string
 * @param fd valid file descriptor
 * @return string containing file content
 */
char* json_get_file_content(int fd) {
    off_t raw_len = lseek(fd, 0, SEEK_END);
    char *raw_ptr = mmap(0, raw_len, PROT_READ, MAP_PRIVATE, fd, 0);
    char *file_content = malloc(raw_len + 1);
    file_content[raw_len] = '\0';
    strncpy(file_content, raw_ptr, raw_len);
    munmap(raw_ptr, raw_len);

    return file_content;
}

/**
 * Creates a new configuration object, creates necessary file if it doesn't exist.
 * @param obj pointer to config object holder
 * @param path path to the config file.
 * @return -1 on error, 0 on success
 */
json_container_t* json_from_file(const char *path) {
    int fd = open(path, O_RDWR | O_CREAT | O_APPEND, 0600);

    if (fd == -1) {
        return NULL;
    }

    if (json_get_file_size(fd) == 0) {
        write(fd, "{}", 2);
    }

    char* file_content = json_get_file_content(fd);
    json_container_t* root = json_from_string(file_content);

    free(file_content);
    close(fd);
    return root;
}

/**
 * Writes a JSON object to disk
 * @param json JSON_VALUE_OBJ to save
 * @param path Path to file that will contain the object
 * @return 0 on error, 1 on success.
 */
int json_save(json_container_t* json, const char* path) {
    if (access(path, F_OK) == 0) {
        if (unlink(path) == -1) {
            return 0;
        }
    }

    int fd = open(path, O_RDWR | O_CREAT, 0600);

    if (fd == -1) {
        return 0;
    }

    json_dump_container(json, 1, fd);
    return 1;
}