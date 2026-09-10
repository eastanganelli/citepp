cmake_minimum_required(VERSION 3.19)
project(app CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Windows must use MinGW-w64, never MSVC --------------------------------
if(WIN32)
    if(MSVC)
        message(FATAL_ERROR
            "MSVC not allowed. Use MinGW-w64:\n"
            "  cmake -B build -G Ninja -DCMAKE_CXX_COMPILER=g++")
    endif()
    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message(FATAL_ERROR "Windows build requires MinGW-w64 (g++).")
    endif()
endif()

include(FetchContent)
FetchContent_Declare(webview
    GIT_REPOSITORY https://github.com/webview/webview
    GIT_TAG 0.12.0)
FetchContent_MakeAvailable(webview)

add_executable(app src/main.cpp)
target_link_libraries(app PRIVATE webview::core)

if(MINGW)
    target_link_options(app PRIVATE -mwindows -static -static-libgcc -static-libstdc++)
    target_link_libraries(app PRIVATE ole32 oleaut32 uuid version shlwapi advapi32 user32)
endif()

# Copy the built React app (frontend/dist) next to the binary as ui/.
# This is a target of its own rather than a POST_BUILD step on `app`: POST_BUILD
# only fires when the executable itself relinks, so a frontend-only change would
# silently leave a stale ui/ next to an up-to-date binary.
add_custom_target(ui ALL
    COMMAND ${CMAKE_COMMAND} -E rm -rf $<TARGET_FILE_DIR:app>/ui
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/frontend/dist $<TARGET_FILE_DIR:app>/ui
    COMMENT "Copying frontend/dist -> ui/")
add_dependencies(ui app)
