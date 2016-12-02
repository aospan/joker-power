cmake_minimum_required(VERSION 2.4.6)

SET(CMAKE_INSTALL_PREFIX /)
add_executable(joker-power main.c)
install(TARGETS joker-power DESTINATION bin)

