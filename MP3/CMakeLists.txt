
cmake_minimum_required(VERSION 3.0.0)

add_library(MP3 OBJECT "Decoder.cpp" "Helper.cpp" "Frame/Frame.cpp" "Frame/Header.cpp" "Frame/SideInformation.cpp")
