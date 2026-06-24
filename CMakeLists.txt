cmake_minimum_required(VERSION 3.10)  # Set your required CMake version
project(Makefile C)                  # Specify the project name and language (C)

set(CMAKE_C_STANDARD 11)               # Specify the C standard you want (e.g., C11)

# Add your C source files
add_executable(Makefile server.c client.c output.c request.c segel.c)      # Replace main.c with your source files
