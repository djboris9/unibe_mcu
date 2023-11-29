cmake_minimum_required(VERSION 3.20.0)
set(BOARD nucleo_f446re)

find_package(Zephyr)
project(tack)


target_sources(app PRIVATE src/display.c src/main.c)