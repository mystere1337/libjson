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
 * Creates a json string, check length to avoid unexpected behavior
 * @param src Source to copy string from
 * @param len Length of the string to copy
 * @return
 */
json_string_t* json_create_string(const char* src, size_t len) {
    json_string_t* str = malloc(sizeof(json_string_t));

    str->len = len;
    str->value = malloc(len);
    strncpy(str->value, src, len);
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
}

/**
 * Frees a JSON setting
 * @param setting JSON setting
 */
void json_free_setting(json_setting_t* setting) {
    free(setting->name);
    json_free_value(setting->value);
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
}

json_setting_t* json_parse_setting(json_token_t* first) {
    json_setting_t* setting = malloc(sizeof(json_setting_t));

    if (first->type != JSON_TOKEN_STRING) {
        // error: expected a string (setting key must be a string)
        json_free_setting(setting);
        return NULL;
    }

    setting->name = malloc(sizeof(json_string_t));
    setting->name->len = first->string->len;
    strncpy(setting->name->value, first->string->value, first->string->len);

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
            json_lexer_remove_first();
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
    json_container_t* container;

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
    return NULL;
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

    json_container_t* root = json_parse_container(TAILQ_FIRST(&json_head));

    json_lexer_clear_list();
    return root;
}

/**
 * Get key count in key_array
 * @param key_array
 * @return number of elements in the array
 */
size_t json_key_count(char** key_array) {
    size_t key_count = 0;

    for (size_t i = 0; key_array[i] != NULL; i++) {
        key_count++;
    }

    return key_count;
}

/**
 * Frees a char** array
 * @param array Array to free from memory
 */
void json_free_double_char_array(char** array) {
    for (int i = 0; array[i] != NULL; i++) {
        free(array[i]);
    }

    free(array);
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
 * Counts number of invisible characters in string
 * @param str JSON_VALUE_STR pointer
 * @return Number of invisible characters in string
 */
size_t json_count_invisible_characters(const char* str) {
    int spaces = 0;
    int quotes = 0;
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\"' && (i == 0 ? 1 : str[i - 1] != '\\')) {
            quotes++;
        }
        if (json_symbol_is_whitespace(str[i]) && !(quotes % 2)) {
            spaces++;
        }
    }

    return spaces;
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
 * Frees a JSON object.
 * @param obj object to free
 */
void json_free(json_obj_t* obj) {
    for (size_t i = 0; i < obj->settings_count; i++) {
        json_free_setting(obj->settings[i]);
    }

    free(obj->settings);
    free(obj);
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
 * Concatenates two strings together
 * @param dest JSON_VALUE_STR to concat with
 * @param src JSON_VALUE_STR to concat on top of dest
 * @return Concatenated string
 */
char* json_strcat(char* dest, const char* src) {
    const size_t a = strlen(dest);
    const size_t b = strlen(src);
    const size_t size_ab = a + b + 1;

    dest = realloc(dest, size_ab + 1);

    memcpy(dest + a, src, b + 1);

    return dest;
}

/**
 * Returns a formatted (Indented... etc.) JSON string from an un-formatted one
 * @param dump Un-formatted JSON string
 * @return Formatted JSON string
 */
char* json_format(const char* str) {
    char* ret = NULL;

    int quotes = 0;
    int braces = 0;
    int count = 0;
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\"' && (i == 0 ? 1 : str[i - 1] != '\\')) {
            if (!(quotes % 2)) {
                for (int k = 0; k < braces; k++) {
                    count++;
                }
            }
            count++;
            quotes++;
            continue;
        }
        if (str[i] == '{' && !(quotes % 2)) {
            braces++;
            count += 2;
            continue;
        }
        if (str[i] == '}' && !(quotes % 2)) {
            braces--;
            count += 2;
            for (int k = 0; k < braces; k++) {
                count++;
            }
        }
        if (str[i] == ':' && !(quotes % 2)) {
            count += 2;
            continue;
        }
        if (str[i] == ',' && !(quotes % 2)) {
            count += 2;
            continue;
        }
        if (json_symbol_is_whitespace(str[i]) && !(quotes % 2)) {
            continue;
        }
        count++;
    }

    ret = malloc(strlen(str) + count + 1);

    int j = 0;
    quotes = 0;
    braces = 0;
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\"' && (i == 0 ? 1 : str[i - 1] != '\\')) {
            if (!(quotes % 2) && str[i - 1] != ':') {
                for (int k = 0; k < braces; k++) {
                    ret[j] = '\t'; j++;
                }
            }
            ret[j] = '\"'; j++;
            quotes++;
            continue;
        }
        if (str[i] == '{' && !(quotes % 2)) {
            braces++;
            ret[j] = '{'; j++;
            if (str[i+1] != '}') {
                ret[j] = '\n'; j++;
            }
            continue;
        }
        if (str[i] == '}' && !(quotes % 2)) {
            braces--;
            if (str[i-1] != '{') {
                ret[j] = '\n'; j++;
                for (int k = 0; k < braces; k++) {
                    ret[j] = '\t'; j++;
                }
            }
            ret[j] = '}'; j++;
            continue;
        }
        if (str[i] == ':' && !(quotes % 2)) {
            ret[j] = ':'; j++;
            ret[j] = ' '; j++;
            continue;
        }
        if (str[i] == ',' && !(quotes % 2)) {
            ret[j] = ','; j++;
            ret[j] = '\n'; j++;
            continue;
        }
        if (json_symbol_is_whitespace(str[i]) && !(quotes % 2)) {
            continue;
        }
        ret[j] = str[i]; j++;
    }
    ret[j] = '\0';

    return ret;
}

/**
 * Creates a human readable string containing the given JSON object
 * @param obj JSON object to dump
 * @return JSON string
 */
char* json_dump(json_obj_t* obj, int format) {
    char* tmp;
    char* str = malloc(2);
    strncpy(str, "{", 2);

    for (size_t i = 0; i < obj->settings_count; i++) {
        size_t needed = snprintf(NULL, 0, "\"%s\":", obj->settings[i]->name) + 1;
        tmp = malloc(needed);
        sprintf(tmp, "\"%s\":", obj->settings[i]->name);
        str = json_strcat(str, tmp);
        free(tmp);

        switch (obj->settings[i]->type) {
            case JSON_VALUE_BOOL: {
                needed = snprintf(NULL, 0, "%s%s", obj->settings[i]->bool_type ? "true" : "false", i == obj->settings_count - 1 ? "" : ",") + 1;
                tmp = malloc(needed);
                sprintf(tmp, "%s%s", obj->settings[i]->bool_type ? "true" : "false", i == obj->settings_count - 1 ? "" : ",");
                str = json_strcat(str, tmp);
                free(tmp);
                break;
            }
            case JSON_VALUE_INT: {
                needed = snprintf(NULL, 0, "%lld%s", obj->settings[i]->long_type, i == obj->settings_count - 1 ? "" : ",") + 1;
                tmp = malloc(needed);
                sprintf(tmp, "%lld%s", obj->settings[i]->long_type, i == obj->settings_count - 1 ? "" : ",");
                str = json_strcat(str, tmp);
                free(tmp);
                break;
            }
            case JSON_VALUE_FLOAT: {
                needed = snprintf(NULL, 0, "%Lf%s", obj->settings[i]->double_type, i == obj->settings_count - 1 ? "" : ",") + 1;
                tmp = malloc(needed);
                sprintf(tmp, "%Lf%s", obj->settings[i]->double_type, i == obj->settings_count - 1 ? "" : ",");
                str = json_strcat(str, tmp);
                free(tmp);
                break;
            }
            case JSON_VALUE_STR: {
                needed = snprintf(NULL, 0, "\"%s\"%s", obj->settings[i]->string_type, i == obj->settings_count - 1 ? "" : ",") + 1;
                tmp = malloc(needed);
                sprintf(tmp, "\"%s\"%s", obj->settings[i]->string_type, i == obj->settings_count - 1 ? "" : ",");
                str = json_strcat(str, tmp);
                free(tmp);
                break;
            }
            case JSON_VALUE_OBJ: {
                if (obj->settings[i]->obj_type == NULL) {
                    needed = snprintf(NULL, 0, "null%s", i == obj->settings_count - 1 ? "" : ",") + 1;
                    tmp = malloc(needed);
                    sprintf(tmp, "null%s", i == obj->settings_count - 1 ? "" : ",");
                    str = json_strcat(str, tmp);
                    free(tmp);
                    break;
                }

                char* dump = json_dump(obj->settings[i]->obj_type, 0);
                needed = snprintf(NULL, 0, "%s%s", dump, i == obj->settings_count - 1 ? "" : ",") + 1;
                tmp = malloc(needed);
                sprintf(tmp, "%s%s", dump, i == obj->settings_count - 1 ? "" : ",");
                str = json_strcat(str, tmp);
                free(tmp);
                free(dump);
                break;
            }
        }
    }

    str = json_strcat(str, "}");

    if (format) {
        char* formatted = json_format(str);
        free(str);
        return formatted;
    } else {
        return str;
    }
}

/**
 * Prints a JSON object on the standard output. Memory handled
 * @param obj JSON_VALUE_OBJ to print
 * @param format JSON_VALUE_BOOL; Format the output (1) or no (0)?
 */
void json_print(json_obj_t* obj, int format) {
    char* dump = json_dump(obj, format);

    printf("%s\n", dump);
    free(dump);
}

/**
 * Writes a JSON object to disk
 * @param json JSON_VALUE_OBJ to save
 * @param path Path to file that will contain the object
 * @return 0 on error, 1 on success.
 */
int json_save(json_obj_t* json, const char* path) {
    if (access(path, F_OK) == 0) {
        if (unlink(path) == -1) {
            return 0;
        }
    }

    int fd = open(path, O_RDWR | O_CREAT, 0600);

    if (fd == -1) {
        return 0;
    }

    char* str = json_dump(json, 0);
    write(fd, str, strlen(str));

    free(str);
    return 1;
}

/**
 * Gets length of key array from string
 * @param str Key array (ex: "object1.object2.setting")
 * @return The number of keys in the string (ex: 3)
 */
size_t json_get_key_array_len(const char* str) {
    size_t count = 0;

    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '.') { count++; }
    }
    return count + 1;
}

/**
 * Splits a string representing array of strings like
 * @param str Key array (ex: "object1.object2.setting")
 * @param separator Separator to split keys in the string
 * @return A NULL-terminated array of key strings
 */
char** json_get_key_array(const char* str, const char separator) {
    size_t len = strlen(str);
    char* double_sep = malloc(3);
    double_sep[0] = separator;
    double_sep[1] = separator;
    double_sep[2] = '\0';

    if (strstr(str, double_sep) != NULL || str[0] == separator || str[len - 1] == separator) {
        free(double_sep);
        return NULL;
    }

    free(double_sep);

    size_t arr_len = json_get_key_array_len(str);
    char** str_array = malloc(sizeof(char*) * (arr_len + 1));
    str_array[arr_len] = NULL;

    for (size_t i = 0, j = 0, k = 0; str[i] != '\0'; i++) {
        if (str[i] == separator || i == len - 1) {
            int offset = (i == len - 1) ? 2 : 1;

            str_array[j] = malloc(i - k + offset);
            str_array[j][i - k + offset - 1] = '\0';
            strncpy(str_array[j], &str[k], i - k + offset - 1);
            k = i + 1;
            j++;
            continue;
        }
    }

    return str_array;
}

/**
 * Get first corresponding string setting
 * @param obj JSON_VALUE_OBJ to search
 * @param key_array Key array
 * @return Corresponding setting or NULL if not found
 */
json_setting_t* json_get_setting(json_obj_t* obj, char** key_array, int remove) {
    if (obj == NULL) {
        return NULL;
    }

    size_t key_count = json_key_count(key_array);

    for (size_t i = 0; i < obj->settings_count; i++) {
        if (strcmp(obj->settings[i]->name, key_array[0]) == 0) {
            if (obj->settings[i]->type == JSON_VALUE_OBJ && key_count - 1 != 0) {
                return json_get_setting(obj->settings[i]->obj_type, &key_array[1], remove);
            } else {
                json_setting_t* ret = obj->settings[i];

                if (remove) {
                    if (i != obj->settings_count - 1) {
                        for (size_t j = 0; j < obj->settings_count - i - 1; j++) {
                            obj->settings[i + j] = obj->settings[i + j + 1];
                        }
                    }

                    json_setting_t** tmp = realloc(obj->settings, (obj->settings_count - 1) * sizeof(json_setting_t*));

                    if (tmp == NULL && obj->settings_count > 1) {
                        exit(1);
                    }

                    obj->settings_count -= 1;
                    obj->settings = tmp;
                }

                return ret;
            }
        }
    }

    return NULL;
}

/**
 * Get corresponding string setting
 * @param obj JSON_VALUE_OBJ to search
 * @param str Key array like (ex: "object.setting")
 * @param separator Char separator separating the keys in the string
 * @return Corresponding setting or NULL if error happens
 */
char* json_get_string(json_obj_t* obj, const char* str, char separator) {
    char** key_array = json_get_key_array(str, separator);
    json_setting_t* setting = json_get_setting(obj, key_array, 0);

    json_free_double_char_array(key_array);

    if (setting == NULL || setting->type != JSON_VALUE_STR) {
        printf("error: can't find %s, or it isn't a string\n", str);
        return NULL;
    }

    return setting->string_type;
}

/**
 * Get corresponding boolean setting
 * @param obj JSON_VALUE_OBJ to search
 * @param str Key array like (ex: "object.setting")
 * @param separator Char separator separating the keys in the string (ex: '.')
 * @return Correct boolean values or 0 if nothing is found
 */
int json_get_bool(json_obj_t* obj, const char* str, char separator) {
    char** key_array = json_get_key_array(str, separator);
    json_setting_t* setting = json_get_setting(obj, key_array, 0);

    json_free_double_char_array(key_array);

    if (setting == NULL || setting->type != JSON_VALUE_BOOL) {
        printf("error: can't find %s, or it isn't a bool\n", str);
        return 0;
    }

    return setting->bool_type;
}

/**
 * Get corresponding integer setting
 * @param obj JSON_VALUE_OBJ to search
 * @param str Key array like (ex: "object.setting")
 * @param separator Char separator separating the keys in the string (ex: '.')
 * @return Corresponding setting or 0 if error happens
 */
long long json_get_integer(json_obj_t* obj, const char* str, char separator) {
    char** key_array = json_get_key_array(str, separator);
    json_setting_t* setting = json_get_setting(obj, key_array, 0);

    json_free_double_char_array(key_array);

    if (setting == NULL || setting->type != JSON_VALUE_INT) {
        printf("error: can't find %s, or it isn't an integer\n", str);
        return 0;
    }

    return setting->long_type;
}

/**
 * Get corresponding object setting
 * @param obj JSON_VALUE_OBJ to search
 * @param str Key array like (ex: "object.setting")
 * @param separator Char separator separating the keys in the string (ex: '.')
 * @return Corresponding setting or NULL if error happens
 */
json_obj_t* json_get_object(json_obj_t* obj, const char* str, char separator) {
    char** key_array = json_get_key_array(str, separator);
    json_setting_t* setting = json_get_setting(obj, key_array, 0);

    json_free_double_char_array(key_array);

    if (setting == NULL || setting->type != JSON_VALUE_OBJ) {
        printf("error: can't find %s, or it isn't an object\n", str);
        return NULL;
    }

    return setting->obj_type;
}

/**
 * Get corresponding floating point number setting
 * @param obj JSON_VALUE_OBJ to search
 * @param str Key array like (ex: "object.setting")
 * @param separator Char separator separating the keys in the string (ex: '.')
 * @return Corresponding values or 0 if error happens
 */
long double json_get_floating(json_obj_t* obj, const char* str, char separator) {
    char** key_array = json_get_key_array(str, separator);
    json_setting_t* setting = json_get_setting(obj, key_array, 0);

    json_free_double_char_array(key_array);

    if (setting == NULL || setting->type != JSON_VALUE_FLOAT) {
        printf("error: can't find %s, or it isn't a floating point number\n", str);
        return 0;
    }

    return setting->double_type;
}

/**
 * Removes a setting from an object at specified key
 * @param obj JSON_VALUE_OBJ to search in
 * @param key Key to setting
 * @param separator Separator
 * @return 0 if obj is NULL, if setting doesn't exist, 1 on success
 */
int json_remove_setting(json_obj_t* obj, const char* key, char separator) {
    if (obj == NULL) {
        return 0;
    }

    char** key_array = json_get_key_array(key, separator);
    json_setting_t* setting = json_get_setting(obj, key_array, 1);

    json_free_double_char_array(key_array);
    json_free_setting(setting);

    return 1;
}

/**
 * Creates a string setting and returns a pointer to it
 * @param name Name of the setting
 * @param value Value of the setting
 * @return The created setting
 */
json_setting_t* json_create_string_setting(const char* name, const char* value) {
    json_setting_t* setting = malloc(sizeof(json_setting_t));

    setting->type = JSON_VALUE_STR;
    setting->name = malloc(strlen(name) + 1);
    strcpy(setting->name, name);
    setting->string_type = malloc(strlen(value) + 1);
    strcpy(setting->string_type, value);

    return setting;
}

/**
 * Creates a boolean setting and returns a pointer to it
 * @param name Name of the setting
 * @param value Value of the setting
 * @return The created setting
 */
json_setting_t* json_create_bool_setting(const char* name, int value) {
    json_setting_t* setting = malloc(sizeof(json_setting_t));

    setting->type = JSON_VALUE_BOOL;
    setting->name = malloc(strlen(name) + 1);
    strcpy(setting->name, name);
    setting->bool_type = value;

    return setting;
}

/**
 * Creates a integer setting and returns a pointer to it
 * @param name Name of the setting
 * @param value Value of the setting
 * @return The created setting
 */
json_setting_t* json_create_integer_setting(const char* name, long long value) {
    json_setting_t* setting = malloc(sizeof(json_setting_t));

    setting->type = JSON_VALUE_INT;
    setting->name = malloc(strlen(name) + 1);
    strcpy(setting->name, name);
    setting->long_type = value;

    return setting;
}

/**
 * Creates a floating point setting and returns a pointer to it
 * @param name Name of the setting
 * @param value Value of the setting
 * @return The created setting
 */
json_setting_t* json_create_floating_setting(const char* name, long double value) {
    json_setting_t* setting = malloc(sizeof(json_setting_t));

    setting->type = JSON_VALUE_FLOAT;
    setting->name = malloc(strlen(name) + 1);
    strcpy(setting->name, name);
    setting->double_type = value;

    return setting;
}

/**
 * Creates a integer setting and returns a pointer to it
 * @param name Name of the setting
 * @param value Value of the setting
 * @return The created setting
 */
json_setting_t* json_create_object_setting(const char* name, json_obj_t* value) {
    json_setting_t* setting = malloc(sizeof(json_setting_t));

    setting->type = JSON_VALUE_OBJ;
    setting->name = malloc(strlen(name) + 1);
    strcpy(setting->name, name);
    setting->obj_type = value;

    return setting;
}

/**
 * Adds a setting to an object. If the object is already containing a setting with this string, it will
 * automatically be overwritten by the new one.
 * @param obj JSON_VALUE_OBJ to which add the setting
 * @param setting Setting to add to the object
 * @param key_array Array of keys containing the path
 * @return 0 on failure, 1 on success
 */
int json_add_setting(json_obj_t* obj, json_setting_t* setting, char** key_array, size_t key_count) {
    if (obj == NULL) {
        return 0;
    }

    json_setting_t* old_setting = json_get_setting(obj, key_array, 1);
    json_free_setting(old_setting);

    for (size_t i = 0; i < obj->settings_count; i++) {
        if (strcmp(obj->settings[i]->name, key_array[0]) == 0 && key_count > 1) {
            if (obj->settings[i]->type == JSON_VALUE_OBJ && key_count - 1 != 0) {
                return json_add_setting(obj->settings[i]->obj_type, setting, &key_array[1], key_count - 1);
            }
        } else {
            json_obj_t* obj_new = malloc(sizeof(json_obj_t));

            obj_new->settings_count = obj->settings_count + 1;
            obj_new->settings = malloc(sizeof(json_setting_t*) * (obj->settings_count + 1));
            for(size_t j = 0; j < obj->settings_count; j++) {
                obj_new->settings[j] = malloc(sizeof(json_setting_t));
                *(obj_new->settings[j]) = *(obj->settings[j]);
            }
            obj_new->settings[obj->settings_count] = setting;

            for(size_t j = 0; j < obj->settings_count; j++) {
                free(obj->settings[j]);
            }
            free(obj->settings);

            *obj = *obj_new;
            free(obj_new);
            return 1;
        }
    }

    return 0;
}

/**
 * Sets a string setting at the desired key
 * @param obj JSON_VALUE_OBJ in which set the setting
 * @param key Key path at which set the setting (ex: object.object.setting)
 * @param separator Separator of keys in key path
 * @param value Value to set in the setting
 * @return 0 on failure, 1 on success. Can be a failure when the object in which to set the setting doesn't exist
 */
int json_set_string(json_obj_t* obj, const char* key, char separator, const char* value) {
    char** key_array = json_get_key_array(key, separator);
    size_t key_count = json_key_count(key_array);
    json_setting_t* setting = json_create_string_setting(key_array[key_count - 1], value);
    int ret = json_add_setting(obj, setting, key_array, key_count);

    json_free_double_char_array(key_array);

    return ret;
}

/**
 * Sets a boolean setting at the desired key
 * @param obj JSON_VALUE_OBJ in which set the setting
 * @param key Key path at which set the setting (ex: object.object.setting)
 * @param separator Separator of keys in key path
 * @param value Value to set in the setting
 * @return 0 on failure, 1 on success. Can be a failure when the object in which to set the setting doesn't exist
 */
int json_set_bool(json_obj_t* obj, const char* key, char separator, int value) {
    char** key_array = json_get_key_array(key, separator);
    size_t key_count = json_key_count(key_array);
    int ret = json_add_setting(obj, json_create_bool_setting(key_array[key_count - 1], value), key_array, key_count);

    json_free_double_char_array(key_array);

    return ret;
}

/**
 * Sets a integer setting at the desired key
 * @param obj JSON_VALUE_OBJ in which set the setting
 * @param key Key path at which set the setting (ex: object.object.setting)
 * @param separator Separator of keys in key path
 * @param value Value to set in the setting
 * @return 0 on failure, 1 on success. Can be a failure when the object in which to set the setting doesn't exist
 */
int json_set_integer(json_obj_t* obj, const char* key, char separator, long long value) {
    char** key_array = json_get_key_array(key, separator);
    size_t key_count = json_key_count(key_array);
    int ret = json_add_setting(obj, json_create_integer_setting(key_array[key_count - 1], value), key_array, key_count);

    json_free_double_char_array(key_array);

    return ret;
}

/**
 * Sets a floating setting at the desired key
 * @param obj JSON_VALUE_OBJ in which set the setting
 * @param key Key path at which set the setting (ex: object.object.setting)
 * @param separator Separator of keys in key path
 * @param value Value to set in the setting
 * @return 0 on failure, 1 on success. Can be a failure when the object in which to set the setting doesn't exist
 */
int json_set_floating(json_obj_t* obj, const char* key, char separator, long double value) {
    char** key_array = json_get_key_array(key, separator);
    size_t key_count = json_key_count(key_array);
    int ret = json_add_setting(obj, json_create_floating_setting(key_array[key_count - 1], value), key_array, key_count);

    json_free_double_char_array(key_array);

    return ret;
}

/**
 * Sets an object setting at the desired key
 * @param obj JSON_VALUE_OBJ in which set the setting
 * @param key Key path at which set the setting (ex: object.object.setting)
 * @param separator Separator of keys in key path
 * @param value Value to set in the setting
 * @return 0 on failure, 1 on success. Can be a failure when the object in which to set the setting doesn't exist
 */
int json_set_object(json_obj_t* obj, const char* key, char separator, json_obj_t* value) {
    char** key_array = json_get_key_array(key, separator);
    size_t key_count = json_key_count(key_array);
    int ret = json_add_setting(obj, json_create_object_setting(key_array[key_count - 1], value), key_array, key_count);

    json_free_double_char_array(key_array);

    return ret;
}