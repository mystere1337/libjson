# libjson

A simple JSON parser and configuration manager.

## Features

### Current features

- Object creation
  - From file
  - From String
- Object storing
- Object freeing
- Object saving to file

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

- Getting value at runtime
- Adding/changing value at runtime
- Add flags for file opening to chose if the user wants to create it automatically
- List type
- Dot support in setting names

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
      "setting": "hello"
    }
  }
}
```

#### Getting a string setting

To get a string at runtime use `json_get_string()`. The function will return NULL if no corresponding setting was found.

```c
char* str = json_get_string(json, "string");

if (str == NULL) {
    printf("error: setting \"string\" doesn't exist\n");
}
```

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
    char* str = json_get_string(json, "string");
    if (str == NULL) {
        printf("error: setting \"string\" doesn't exist\n");
        json_free(json);
        return 1;
    }
    
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