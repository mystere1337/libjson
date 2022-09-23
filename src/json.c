#include "json.h"

#include <fcntl.h>
#include <malloc.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>

/**
 * Gets file size in bytes
 * @param fd File descriptor
 * @return Size of file in bytes
 */
size_t json_get_file_size(int fd) {
    struct stat s;

    int status = fstat(fd, &s);
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
    size_t count = 0;

    int quotes = 0;
    int braces = 0;
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\"' && (i == 0 ? 1 : str[i - 1] != '\\')) { quotes++; }
        if (str[i] == '{' && !(quotes % 2)) { braces++; }
        if (str[i] == '}' && !(quotes % 2)) { braces--; }
        if (str[i] == ':' && !(quotes % 2) && !braces) {
            count++;
        }
    }

    return count;
}

/**
 * Deserializes unique json setting
 * @param string Serialized setting of type "key":"value"
 * @return -1 on error, 0 on success
 */
setting_t* parse_setting_line(char *string) {
    setting_t* set = malloc(sizeof(setting_t));
    size_t len = strlen(string);
    char* str_value;

    int found = 0;
    int quotes = 0;
    int colon = 0;
    for (int i = 0; i < len; i++) {
        if (string[i] == '\"' && (i == 0 ? 1 : string[i - 1] != '\\')) { quotes++; }
        if (string[i] == ':' && !(quotes % 2) && !found) {
            found = 1;
            colon = i;
            set->name = malloc(i - 1);
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
    } else if (str_value[0] == '-' || str_value[0] >= '0' && str_value[0] <= '9') {
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
        printf("error: setting '%s' is of an unknown type\n", string);
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
 * @return Array of json settings
 */
char **json_get_string_settings(const char *str) {
    char* aligned = json_align(str);
    char* isolated = json_isolate_content(aligned);
    size_t isolated_len = strlen(isolated);
    size_t setting_count = json_count_isolated_settings(isolated);

    char** setting_strings = malloc(sizeof(char*) * (setting_count + 1));
    setting_strings[setting_count] = NULL;

    int quotes = 0;
    int braces = 0;
    int prev_pos = 0;
    int str_index = 0;
    for (int i = 0; isolated[i] != '\0'; i++) {
        if (isolated[i] == '\"' && (i == 0 ? 1 : isolated[i - 1] != '\\')) { quotes++; }
        if (isolated[i] == '{' && !(quotes % 2)) { braces++; }
        if (isolated[i] == '}' && !(quotes % 2)) { braces--; }
        if ((isolated[i] == ',' && !(quotes % 2) && !braces) || i == isolated_len - 1) {
            int offset = i == isolated_len - 1 ? 2 : 1;

            setting_strings[str_index] = malloc(i - prev_pos + offset);
            setting_strings[str_index][i - prev_pos + offset - 1] = '\0';
            strncpy(setting_strings[str_index], &isolated[prev_pos], i == isolated_len - 1 ? i + 1 - prev_pos : i - prev_pos);

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
obj_t* json_from_string(const char* str) {
    obj_t* obj = malloc(sizeof(obj_t));

    obj->settings = NULL;
    obj->settings_count = (size_t)0;

    char** settings = json_get_string_settings(str);

    for (int i = 0; settings[i] != NULL; i++) {
        obj->settings_count++;
    }

    if (obj->settings_count) {
        obj->settings = malloc(sizeof(setting_t) * obj->settings_count);

        for (int i = 0; i < obj->settings_count; i++) {
            obj->settings[i] = parse_setting_line(settings[i]);
        };
    }

    for (size_t i = 0; settings[i] != NULL; i++) {
        free(settings[i]);
    }
    free(settings);
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
void json_free(obj_t* obj) {
    for (size_t i = 0; i < obj->settings_count; i++) {
        free(obj->settings[i]->name);

        if (obj->settings[i]->type == String) {
            free(obj->settings[i]->string_type);
        }

        if (obj->settings[i]->type == Object && obj->settings[i]->obj_type != NULL) {
            json_free(obj->settings[i]->obj_type);
        }

        free(obj->settings[i]);
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
obj_t* json_from_file(const char *path) {
    int fd = open(path, O_RDWR | O_CREAT | O_APPEND, 0600);

    if (fd == -1) {
        return NULL;
    }

    if (json_get_file_size(fd) == 0) {
        write(fd, "{}", 2);
    }

    char* file_content = json_get_file_content(fd);
    obj_t* obj = json_from_string(file_content);

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
char* json_dump(obj_t* obj) {
    char* tmp;
    char* str = malloc(2);
    strncpy(str, "{", 1);

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

                char* dump = json_dump(obj->settings[i]->obj_type);
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
    return str;
}

/**
 * Writes a JSON object to disk
 * @param json Object to save
 * @param path Path to file that will contain the object
 * @return -1 on error, 0 on success.
 */
int json_save(obj_t* json, const char* path) {
    if (access(path, F_OK) == 0) {
        if (unlink(path) == -1) {
            return -1;
        }
    }

    int fd = open(path, O_RDWR | O_CREAT, 0600);

    if (fd == -1) {
        return -1;
    }

    char* str = json_dump(json);
    char* formatted = json_format(str);
    write(fd, formatted, strlen(formatted));

    free(str);
    free(formatted);
    return 0;
}
