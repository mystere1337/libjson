:warning: The project is still in development and may contain
memory leaks or critical bugs.

# libjson

A simple JSON parser and configuration manager. Minimal.

## Features

### Current features

- Object creation
  - From file
  - From String
- Object storing
- Object freeing
- Object saving to file
- Getting setting value at runtime
- Dot support in setting names

### Supported types

Libjson is a small personal project, and more types will be supported in the future.
You can also contribute to the project if you're interested. For now, the supported types are:
- `Integers`
- `Floating point numbers`
- `Strings`
- `Booleans`
- `JSON Objects`

Types to be added are:
- `Lists`

### Planned features

- Adding/changing value at runtime
- Add flags for file opening to chose if the user wants to create it automatically
- List type
- Better way to check for errors

## Documentation

### Objects

To store an object at runtime, use the struct `obj_t`. An `obj_t` contains individual settings that can be of any supported type.

### Creating and loading objects

Creating an object is the only way to initialize an `obj_t`.

#### From file

Note: *Opening from a file creates the corresponding file if it doesn't exists*

```c
obj_t* json = json_from_file("./object.json");

if (json == NULL) {
    printf("error: invalid configuration\n");
    return 1;
}
```

#### From string

```c
obj_t* json = json_from_string("{\"key\":\"value\"}");

if (json == NULL) {
    printf("error: invalid configuration\n");
    return 1;
}
```

### Getting settings at runtime

Any function that gets a setting will return the desired type, and will need an `obj_t` parameter and the setting identifier of form `objX.objY.setting` as second parameter. Example:

```json
{
  "objX": {
    "objY": {
      "setting": "hello",
      "setting.2": 69
    }
  }
}
```

Note: It is possible to get settings from keys that are containing dots, for this you need to specify the separator as the third parameter, `objX|objY|setting.2` with `|` separator will return `69` as the result, following the example above.

#### Getting a string setting

To get a string at runtime use `json_get_string()`. The function will return NULL if no corresponding setting was found.

```c
char* str = json_get_string(json, "some_string", '.');
```

#### Getting an integer setting

To get an integer at runtime use `json_get_integer()`.

```c
long long integer = json_get_integer(json, "some_integer", '.');
```

Note: The function will return 0 if no corresponding setting was found. Other than an unexpected value, there is currently no way to distinguish if the value could be fetched or not.

#### Getting a boolean setting

To get a boolean at runtime use `json_get_boolean()`.

```c
int boolean = json_get_bool(json, "some_boolean", '.');
```

Note: The function will return 0 if no corresponding setting was found. Other than an unexpected value, there is currently no way to distinguish if the value could be fetched or not.

#### Getting a floating point number setting

To get a floating point number at runtime use `json_get_floating()`.

```c
long double floating = json_get_floating(json, "some_floating", '.');
```

Note: The function will return 0 if no corresponding setting was found. Other than an unexpected value, there is currently no way to distinguish if the value could be fetched or not.

#### Getting a JSON object setting

To get a JSON object at runtime use `json_get_floating()`.

```c
obj_t* obj = json_get_object(json, "some_object", '.');
```

Note: The function will return NULL if no corresponding setting was found.

### Saving object to file

You can save any JSON object to the desired file.

```c
if (json_save(json, "./object.json") == -1) {
    printf("error: failed to save configuration\n");
}
```

### Freeing objects

To avoid memory leaks, after using objects, the user needs to free memory

```c
json_free(json);
```

## Full working example

```c
int main() {
    /* Initialize the root object */
    obj_t* json = json_from_file("./object.json");
    if (json == NULL) {
        printf("error: invalid configuration\n");
        return 1;
    }
    
    /* Get string setting in the root object from it's name */
    char* str = json_get_string(json, "some_str", '.');
    
    /* Get integer setting in the object named 'obj' inside root object */
    long long some_int = json_get_int(json, "obj.some_int", '.');
    
    /* Get boolean setting in the object named 'obj' inside root object */
    int some_bool = json_get_bool(json, "obj#some_bool", '#');
    
    /* Get floating point number setting in an object named 'abc.xyz' */
    long double some_float = json_get_string(json, "abc.xyz#some_float", '#');
    
    /* Get object setting inside of root object*/
    obj_t* obj = json_get_string(json, "some_obj", '.');
    
    /* Save the initialized object to a file */
    if (json_save(json, "./object.json") == -1) {
        printf("error: failed to save configuration\n");
    }

    /* Free memory containing the object */
    json_free(json);
    return 0;
}
```

## Development

```shell
git clone git@github.com:mystere1337/libjson.git
cd libjson
```