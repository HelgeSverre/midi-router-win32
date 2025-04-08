cmake_minimum_required(VERSION 3.30)
project(midi_router_win32 C)
set(CMAKE_C_STANDARD 11)

# Add resource file compilation for Windows
set(RC_FILE midi_router.rc)
set(CMAKE_RC_COMPILER windres)
enable_language(RC)

# Add executable with source and resource files
add_executable(midi_router_win32 midi_router.c ${RC_FILE})

# Link required Windows libraries
target_link_libraries(midi_router_win32 winmm comctl32)