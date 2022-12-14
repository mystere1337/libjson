#include "json.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>

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
 * Checks if current character is transparent
 * @param c Character to check
 * @return Boolean value
 */
int json_is_invisible(const char c) {
    return c == ' ' || c == '\t' || c == '\v' || c == '\r' || c == '\n' || c == '\f';
}

/**
 * Counts number of invisible characters in string
 * @param str String pointer
 * @return Number of invisible characters in string
 */
size_t json_count_invisible_characters(const char* str) {
    int spaces = 0;
    int quotes = 0;
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\"' && (i == 0 ? 1 : str[i - 1] != '\\')) {
            quotes++;
        }
        if (json_is_invisible(str[i]) && !(quotes % 2)) {
            spaces++;
        }
    }

    return spaces;
}

/**
 * Get contents of an object between first and last braces
 * @param str Serialized JSON object
 * @return Content of object (without containing braces)
 */
char* json_isolate_content(const char *str) {
    size_t len = strlen(str);
    char* dest = malloc(len - 1);

    dest[len - 2] = '\0';
    strncpy(dest, str + 1, len - 2);

    return dest;
}

/**
 * Gets number of settings in isolated JSON object
 * @param str Isolated json object
 * @return Number of settings in string
 */
size_t json_count_isolated_settings(const char *str) {
    size_t count = 1;

    int quotes = 0;
    int braces = 0;
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\"' && (i == 0 ? 1 : str[i - 1] != '\\')) { quotes++; }
        if (str[i] == '{' && !(quotes % 2)) { braces++; }
        if (str[i] == '}' && !(quotes % 2)) { braces--; }
        if (str[i] == ',' && !(quotes % 2) && !braces && str[i+1] == '"') {
            count++;
        }
    }

    return count;
}

/**
 * Frees a single setting from memory
 * @param setting Setting object to free
 */
void json_free_setting(json_setting_t *setting) {
    if (setting == NULL) {
        return;
    }

    free(setting->name);

    if (setting->type == String) {
        free(setting->string_type);
    } else if (setting->type == Object && setting->obj_type != NULL) {
        json_free(setting->obj_type);
    }

    free(setting);
}

/**
 * Deserializes unique json setting
 * @param string Serialized setting of type "key":"value"
 * @return -1 on error, 0 on success
 */
json_setting_t* parse_setting_line(const char *string) {
    json_setting_t* set = malloc(sizeof(json_setting_t));
    size_t len = strlen(string);
    char* str_value;

    int found = 0;
    int quotes = 0;
    int mallocced = 0;
    size_t colon = 0;
    for (size_t i = 0; i < len; i++) {
        if (string[i] == '\"' && (i == 0 ? 1 : string[i - 1] != '\\')) { quotes++; }
        if (string[i] == ',' && i == len - 1) {
            if (mallocced == 1) {
                free(set->name);
            }
            free(set);
            return NULL;
        }
        if (string[i] == ':' && !(quotes % 2) && !found && string[i - 1] == '\"') {
            found = 1;
            colon = i;
            set->name = malloc(i - 1);
            mallocced = 1;
            set->name[i - 2] = '\0';
            strncpy(set->name, &string[1], i - 2);
        }
        if (i == len - 1) {
            str_value = malloc(i - colon + 1);
            str_value[i - colon] = '\0';
            strncpy(str_value, &string[colon + 1], i - colon);
            break;
        }
    }

    if (str_value[0] == '\"') {
        size_t len2 = strlen(str_value);
        set->type = String;
        set->string_type = malloc(len2 - 1);
        set->string_type[len2 - 2] = '\0';
        strncpy(set->string_type, &str_value[1], len2 - 2);
    } else if (str_value[0] == 't' || str_value[0] == 'f') {
        set->type = Boolean;
        set->bool_type = strcmp(str_value, "true") == 0 ? 1 : 0;
    } else if (str_value[0] == '-' || (str_value[0] >= '0' && str_value[0] <= '9')) {
        if (strchr(str_value, '.')) {
            set->type = Floating;
            set->double_type = strtod(str_value, NULL);
        } else {
            set->type = Integer;
            set->long_type = strtoll(str_value, NULL, 10);
        }
    } else if (str_value[0] == '{') {
        set->type = Object;
        set->obj_type = json_from_string(str_value);
    } else if (str_value[0] == 'n') {
        set->type = Object;
        set->obj_type = NULL;
    } else {
        free(set->name);
        free(set);
        free(str_value);
        return NULL;
    }

    free(str_value);
    return set;
}

/**
 * Clears invisible characters in string for further processing
 * @param str Pointer to string to clear
 * @return Cleared string pointer
 */
char* json_align(const char* str) {
    size_t len = strlen(str);
    size_t spaces = json_count_invisible_characters(str);
    char *aligned = malloc(len - spaces + 1);
    aligned[len - spaces] = '\0';

    int j = 0;
    int quotes = 0;
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\"' && (i == 0 ? 1 : str[i - 1] != '\\')) { quotes++; }
        if (json_is_invisible(str[i]) && !(quotes % 2)) { continue; }
        aligned[j] = str[i];
        j++;
    }

    return aligned;
}

/**
 * Extracts strings from a serialized JSON object, each containing one setting of the object
 * @param str Serialized JSON string
 * @return Array of json setting names
 */
char** json_get_string_settings(const char *str) {
    char* aligned = json_align(str);
    size_t len = strlen(aligned);

    if (str[0] != '{' || str[len - 1] != '}') {
        free(aligned);
        return NULL;
    }

    char* isolated = json_isolate_content(aligned);
    size_t setting_count = json_count_isolated_settings(isolated);

    if (setting_count < 1 && len > 2) {
        free(aligned);
        free(isolated);
        return NULL;
    }

    char** setting_strings = malloc(sizeof(char*) * (setting_count + 1));
    setting_strings[setting_count] = NULL;

    int quotes = 0;
    int braces = 0;
    size_t prev_pos = 0;
    int str_index = 0;
    for (size_t i = 0; isolated[i] != '\0'; i++) {
        if (isolated[i] == '\"' && (i == 0 ? 1 : isolated[i - 1] != '\\')) { quotes++; }
        if (isolated[i] == '{' && !(quotes % 2)) { braces++; }
        if (isolated[i] == '}' && !(quotes % 2)) { braces--; }
        if ((isolated[i] == ',' && !(quotes % 2) && !braces) || i == len - 3) {
            int offset = i == len - 3 ? 2 : 1;

            setting_strings[str_index] = malloc(i - prev_pos + offset);
            setting_strings[str_index][i - prev_pos + offset - 1] = '\0';
            strncpy(setting_strings[str_index], &isolated[prev_pos], i == len - 3 ? i + 1 - prev_pos : i - prev_pos);

            str_index++;
            prev_pos = i + 1;
        }
    }

    free(aligned);
    free(isolated);
    return setting_strings;
}

/**
 * Converts a serialized JSON string to an object.
 * @param obj pointer to JSON object
 * @param str JSON string
 * @return -1 if unsuccessful, 0 if successful
 */
json_obj_t* json_from_string(const char* str) {
    char** settings = json_get_string_settings(str);

    if (settings == NULL) {
        return NULL;
    }

    json_obj_t* obj = malloc(sizeof(json_obj_t));

    obj->settings = NULL;
    obj->settings_count = (size_t)0;

    for (int i = 0; settings[i] != NULL; i++) {
        obj->settings_count++;
    }

    if (obj->settings_count) {
        obj->settings = calloc(sizeof(json_setting_t) * obj->settings_count, sizeof(json_setting_t*));

        for (size_t i = 0; i < obj->settings_count; i++) {
            json_setting_t* setting = parse_setting_line(settings[i]);

            if (setting == NULL) {
                for (size_t j = 0; j < obj->settings_count; j++) {
                    json_free_setting(obj->settings[j]);
                }
                free(obj->settings);
                free(obj);

                json_free_double_char_array(settings);
                return NULL;
            }
            obj->settings[i] = setting;
        }
    }

    json_free_double_char_array(settings);
    return obj;
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
json_obj_t* json_from_file(const char *path) {
    int fd = open(path, O_RDWR | O_CREAT | O_APPEND, 0600);

    if (fd == -1) {
        return NULL;
    }

    if (json_get_file_size(fd) == 0) {
        write(fd, "{}", 2);
    }

    char* file_content = json_get_file_content(fd);
    json_obj_t* obj = json_from_string(file_content);

    free(file_content);
    close(fd);
    return obj;
}

/**
 * Concatenates two strings together
 * @param dest String to concat with
 * @param src String to concat on top of dest
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
        if (json_is_invisible(str[i]) && !(quotes % 2)) {
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
        if (json_is_invisible(str[i]) && !(quotes % 2)) {
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
            case Boolean: {
                needed = snprintf(NULL, 0, "%s%s", obj->settings[i]->bool_type ? "true" : "false", i == obj->settings_count - 1 ? "" : ",") + 1;
                tmp = malloc(needed);
                sprintf(tmp, "%s%s", obj->settings[i]->bool_type ? "true" : "false", i == obj->settings_count - 1 ? "" : ",");
                str = json_strcat(str, tmp);
                free(tmp);
                break;
            }
            case Integer: {
                needed = snprintf(NULL, 0, "%lld%s", obj->settings[i]->long_type, i == obj->settings_count - 1 ? "" : ",") + 1;
                tmp = malloc(needed);
                sprintf(tmp, "%lld%s", obj->settings[i]->long_type, i == obj->settings_count - 1 ? "" : ",");
                str = json_strcat(str, tmp);
                free(tmp);
                break;
            }
            case Floating: {
                needed = snprintf(NULL, 0, "%Lf%s", obj->settings[i]->double_type, i == obj->settings_count - 1 ? "" : ",") + 1;
                tmp = malloc(needed);
                sprintf(tmp, "%Lf%s", obj->settings[i]->double_type, i == obj->settings_count - 1 ? "" : ",");
                str = json_strcat(str, tmp);
                free(tmp);
                break;
            }
            case String: {
                needed = snprintf(NULL, 0, "\"%s\"%s", obj->settings[i]->string_type, i == obj->settings_count - 1 ? "" : ",") + 1;
                tmp = malloc(needed);
                sprintf(tmp, "\"%s\"%s", obj->settings[i]->string_type, i == obj->settings_count - 1 ? "" : ",");
                str = json_strcat(str, tmp);
                free(tmp);
                break;
            }
            case Object: {
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
 * @param obj Object to print
 * @param format Boolean; Format the output (1) or no (0)?
 */
void json_print(json_obj_t* obj, int format) {
    char* dump = json_dump(obj, format);

    printf("%s\n", dump);
    free(dump);
}

/**
 * Writes a JSON object to disk
 * @param json Object to save
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
 * @param obj Object to search
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
            if (obj->settings[i]->type == Object && key_count - 1 != 0) {
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
 * @param obj Object to search
 * @param str Key array like (ex: "object.setting")
 * @param separator Char separator separating the keys in the string
 * @return Corresponding setting or NULL if error happens
 */
char* json_get_string(json_obj_t* obj, const char* str, char separator) {
    char** key_array = json_get_key_array(str, separator);
    json_setting_t* setting = json_get_setting(obj, key_array, 0);

    json_free_double_char_array(key_array);

    if (setting == NULL || setting->type != String) {
        printf("error: can't find %s, or it isn't a string\n", str);
        return NULL;
    }

    return setting->string_type;
}

/**
 * Get corresponding boolean setting
 * @param obj Object to search
 * @param str Key array like (ex: "object.setting")
 * @param separator Char separator separating the keys in the string (ex: '.')
 * @return Correct boolean value or 0 if nothing is found
 */
int json_get_bool(json_obj_t* obj, const char* str, char separator) {
    char** key_array = json_get_key_array(str, separator);
    json_setting_t* setting = json_get_setting(obj, key_array, 0);

    json_free_double_char_array(key_array);

    if (setting == NULL || setting->type != Boolean) {
        printf("error: can't find %s, or it isn't a bool\n", str);
        return 0;
    }

    return setting->bool_type;
}

/**
 * Get corresponding integer setting
 * @param obj Object to search
 * @param str Key array like (ex: "object.setting")
 * @param separator Char separator separating the keys in the string (ex: '.')
 * @return Corresponding setting or 0 if error happens
 */
long long json_get_integer(json_obj_t* obj, const char* str, char separator) {
    char** key_array = json_get_key_array(str, separator);
    json_setting_t* setting = json_get_setting(obj, key_array, 0);

    json_free_double_char_array(key_array);

    if (setting == NULL || setting->type != Integer) {
        printf("error: can't find %s, or it isn't an integer\n", str);
        return 0;
    }

    return setting->long_type;
}

/**
 * Get corresponding object setting
 * @param obj Object to search
 * @param str Key array like (ex: "object.setting")
 * @param separator Char separator separating the keys in the string (ex: '.')
 * @return Corresponding setting or NULL if error happens
 */
json_obj_t* json_get_object(json_obj_t* obj, const char* str, char separator) {
    char** key_array = json_get_key_array(str, separator);
    json_setting_t* setting = json_get_setting(obj, key_array, 0);

    json_free_double_char_array(key_array);

    if (setting == NULL || setting->type != Object) {
        printf("error: can't find %s, or it isn't an object\n", str);
        return NULL;
    }

    return setting->obj_type;
}

/**
 * Get corresponding floating point number setting
 * @param obj Object to search
 * @param str Key array like (ex: "object.setting")
 * @param separator Char separator separating the keys in the string (ex: '.')
 * @return Corresponding value or 0 if error happens
 */
long double json_get_floating(json_obj_t* obj, const char* str, char separator) {
    char** key_array = json_get_key_array(str, separator);
    json_setting_t* setting = json_get_setting(obj, key_array, 0);

    json_free_double_char_array(key_array);

    if (setting == NULL || setting->type != Floating) {
        printf("error: can't find %s, or it isn't a floating point number\n", str);
        return 0;
    }

    return setting->double_type;
}

/**
 * Removes a setting from an object at specified key
 * @param obj Object to search in
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

    setting->type = String;
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

    setting->type = Boolean;
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

    setting->type = Integer;
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

    setting->type = Floating;
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

    setting->type = Object;
    setting->name = malloc(strlen(name) + 1);
    strcpy(setting->name, name);
    setting->obj_type = value;

    return setting;
}

/**
 * Adds a setting to an object. If the object is already containing a setting with this name, it will
 * automatically be overwritten by the new one.
 * @param obj Object to which add the setting
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
            if (obj->settings[i]->type == Object && key_count - 1 != 0) {
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
 * @param obj Object in which set the setting
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
 * @param obj Object in which set the setting
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
 * @param obj Object in which set the setting
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
 * @param obj Object in which set the setting
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
 * @param obj Object in which set the setting
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