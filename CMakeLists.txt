cmake_minimum_required(VERSION 3.12)

project(OpenCL LANGUAGES C)

find_package(OpenCLHeaders REQUIRED)
find_package(OpenCL REQUIRED)

add_executable(array_addition array_addition.c)
target_link_libraries(array_addition PRIVATE OpenCL::Headers OpenCL::OpenCL)

add_executable(template_program template_program.c)
target_link_libraries(template_program PRIVATE OpenCL::Headers OpenCL::OpenCL)

