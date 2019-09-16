cmake_minimum_required(VERSION 3.13)
project(NSCC C)

set(CMAKE_C_STANDARD 11)

add_library(NSCC SHARED nscc.c nscc.h)