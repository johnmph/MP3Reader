cmake_minimum_required(VERSION 3.0.0)

project(MP3Reader VERSION 0.1.1)

add_subdirectory("portaudio" EXCLUDE_FROM_ALL)
add_subdirectory(MP3)

add_executable(MP3Reader "main.cpp")
target_include_directories(MP3Reader PRIVATE "portaudio/include")
target_link_libraries(MP3Reader PRIVATE "portaudio" MP3)

target_compile_features(MP3Reader PRIVATE cxx_std_14)