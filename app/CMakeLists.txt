cmake_minimum_required(VERSION 3.20.0)
set(BOARD nucleo_f446re)

find_package(Zephyr)
include(${ZEPHYR_BASE}/cmake/cfb.cmake NO_POLICY_SCOPE) # CFB custom font dependency
project(tack)

target_sources(app PRIVATE src/c12832a1z_display.c src/magneto.c src/gps.c src/main.c)

# CFB custom font dependencies
set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)
generate_cfb_font_for_target(app fonts/Ac437_IBM_CGAthin.ttf ${gen_dir}/cfb_font_gcathin.h 7 8 -s 8)