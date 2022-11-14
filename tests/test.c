#include "test.h"
#include "../src/json.h"

#include <assert.h>
#include <stdio.h>

void is_number_tests() {
    printf("json_symbol_is_number: ");
    assert(json_symbol_is_number("-,") == 0);
    assert(json_symbol_is_number("-10e-10.10,") == 0);
    assert(json_symbol_is_number("-10e10.10,") == 0);
    assert(json_symbol_is_number("-10e+10.10,") == 0);
    assert(json_symbol_is_number("-10E+10.10,") == 0);
    assert(json_symbol_is_number("-10E+10.10a,") == 0);
    assert(json_symbol_is_number("-10E+10.10a  ,") == 0);
    assert(json_symbol_is_number("-10E+10.10a  ,") == 0);
    assert(json_symbol_is_number("null,") == 0);
    assert(json_symbol_is_number("true,") == 0);
    assert(json_symbol_is_number("false,") == 0);
    assert(json_symbol_is_number("\"string\",") == 0);
    assert(json_symbol_is_number("[],") == 0);
    assert(json_symbol_is_number("{},") == 0);
    assert(json_symbol_is_number("   10,") == 0);
    assert(json_symbol_is_number("-1 0,") == 1);
    assert(json_symbol_is_number("- 10,") == 1);
    assert(json_symbol_is_number("-10,") == 1);
    assert(json_symbol_is_number("10,") == 1);
    assert(json_symbol_is_number("10.10,") == 1);
    assert(json_symbol_is_number("-10.10,") == 1);
    assert(json_symbol_is_number("-10e10,") == 1);
    assert(json_symbol_is_number("-10e-10,") == 1);
    assert(json_symbol_is_number("-10e+10,") == 1);
    assert(json_symbol_is_number("-10E+10,") == 1);
    assert(json_symbol_is_number("-10E+10 ,") == 1);
    printf("all tests passed\n");
}

void is_null_tests() {
    printf("json_symbol_is_null: ");
    assert(json_symbol_is_null("-,") == 0);
    assert(json_symbol_is_null("null,") == 1);
    assert(json_symbol_is_null("nul") == 0);
    printf("all tests passed\n");
}

void is_bool_tests() {
    printf("json_symbol_is_bool: ");
    assert(json_symbol_is_bool("-,") == 0);
    assert(json_symbol_is_bool("null,") == 0);
    assert(json_symbol_is_bool("nul") == 0);
    assert(json_symbol_is_bool("tru") == 0);
    assert(json_symbol_is_bool("true,") == 1);
    assert(json_symbol_is_bool("false,") == 1);
    assert(json_symbol_is_bool("false") == 1);
    printf("all tests passed\n");
}

void is_string_tests() {
    printf("json_symbol_is_string: ");
    assert(json_symbol_is_string("-,") == 0);
    assert(json_symbol_is_string("null,") == 0);
    assert(json_symbol_is_string("nul") == 0);
    assert(json_symbol_is_string("tru") == 0);
    assert(json_symbol_is_string("true,") == 0);
    assert(json_symbol_is_string("false,") == 0);
    assert(json_symbol_is_string("false") == 0);
    assert(json_symbol_is_string("\"\"") == 1);
    assert(json_symbol_is_string("\"aaa\"\"") == 1);
    assert(json_symbol_is_string("\"fzefezf\"") == 1);
    assert(json_symbol_is_string("\"fzefe\n\tzf\"") == 1);
    assert(json_symbol_is_string("\"") == 0);
    printf("all tests passed\n");
}

int main() {
    //is_number_tests();
    //is_null_tests();
    //is_bool_tests();
    //is_string_tests();

    json_container_t* root = json_from_string("{\"test\":-634e+12, \"string\":\"hello\", \"bool\":tru, \"null\":null}");

    // json_free(root);
}
