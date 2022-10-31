cmake_minimum_required(VERSION 3.23)
project(libjson C)

set(CMAKE_C_STANDARD 23)

add_library(libjson src/json.c)
