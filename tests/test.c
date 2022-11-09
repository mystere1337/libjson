#include "../src/json.h"

int main() {
    json_obj_t* correct_format = json_from_file("./assets/correct_format.json");

    json_free(correct_format);
}
