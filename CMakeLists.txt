cmake_minimum_required(VERSION 3.23)
project(libjson C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3 -fsanitize=address,undefined -Wall -Wextra -Wsign-compare")

add_library(libjson src/json.c)
