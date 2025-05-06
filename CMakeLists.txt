cmake_minimum_required(VERSION 3.21)

# Tell CMake ≥3.28 to use NEW behaviour for file timestamps so we avoid
# the CMP0135 developer warning.
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

project(heic_demo LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
include(ExternalProject)

# --- Make the toolchain path absolute so nested CMake calls can find it ---
get_filename_component(TOOLCHAIN_FILE_ABS "${CMAKE_TOOLCHAIN_FILE}" REALPATH BASE_DIR "${CMAKE_SOURCE_DIR}")

# -------- ImGui, GLFW, stb_image via FetchContent --------
FetchContent_Declare(imgui  GIT_REPOSITORY https://github.com/ocornut/imgui.git  GIT_TAG v1.90.4  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_Declare(glfw   GIT_REPOSITORY https://github.com/glfw/glfw.git    GIT_TAG 3.3.9    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
# stb_image is a single header; grab it via the whole stb repo ZIP so CMake can
# treat it as an archive (avoids the "Unrecognized archive format" error).
FetchContent_Declare(stb_image
  URL https://github.com/nothings/stb/archive/refs/heads/master.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(imgui glfw stb_image)

# -------- Third‑party static libs install prefix --------
set(THIRD_PARTY_INSTALL ${CMAKE_BINARY_DIR}/third_party)
file(MAKE_DIRECTORY ${THIRD_PARTY_INSTALL})

# -------- x265 (static) --------
ExternalProject_Add(x265
  GIT_REPOSITORY https://bitbucket.org/multicoreware/x265_git.git
  GIT_TAG 3.5
  SOURCE_SUBDIR  source
  CMAKE_ARGS
    -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE_ABS}
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DENABLE_SHARED=OFF
    -DENABLE_PIC=OFF
    -DENABLE_CLI=OFF
    -DCMAKE_INSTALL_PREFIX=${THIRD_PARTY_INSTALL}
  INSTALL_DIR ${THIRD_PARTY_INSTALL}
  BUILD_BYPRODUCTS ${THIRD_PARTY_INSTALL}/lib/libx265.a)

# -------- libheif (static, with x265) --------
ExternalProject_Add(libheif
  GIT_REPOSITORY https://github.com/strukturag/libheif.git
  GIT_TAG v1.17.6
  CMAKE_ARGS
    -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE_ABS}
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DBUILD_SHARED_LIBS=OFF
    -DWITH_X265=ON
    -DX265_INCLUDE_DIR=${THIRD_PARTY_INSTALL}/include
    -DX265_LIBRARY=${THIRD_PARTY_INSTALL}/lib/libx265.a
    -DCMAKE_INSTALL_PREFIX=${THIRD_PARTY_INSTALL}
  DEPENDS x265
  INSTALL_DIR ${THIRD_PARTY_INSTALL}
  BUILD_BYPRODUCTS ${THIRD_PARTY_INSTALL}/lib/libheif.a)

# -------- Imported static libs we can actually link --------
# (ExternalProject creates UTILITY targets, which CANNOT be linked.)
add_library(x265_static STATIC IMPORTED GLOBAL)
set_target_properties(x265_static PROPERTIES
  IMPORTED_LOCATION ${THIRD_PARTY_INSTALL}/lib/libx265.a)
add_dependencies(x265_static x265)

add_library(heif_static STATIC IMPORTED GLOBAL)
set_target_properties(heif_static PROPERTIES
  IMPORTED_LOCATION ${THIRD_PARTY_INSTALL}/lib/libheif.a)
add_dependencies(heif_static libheif)

# ---- Dear ImGui core .cpp files (required – they hold all symbols) ----
set(IMGUI_CORE_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    # optional demo window:
    # ${imgui_SOURCE_DIR}/imgui_demo.cpp
)


# -------- Demo App --------
add_executable(heic_demo WIN32
    src/main.cpp
    ${IMGUI_CORE_SOURCES}
    ${stb_image_SOURCE_DIR}/stb_image.h
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
    
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  set_target_properties(heic_demo PROPERTIES WIN32_EXECUTABLE YES)
endif()

# include directories
 target_include_directories(heic_demo PRIVATE
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    $<BUILD_INTERFACE:${glfw_SOURCE_DIR}/include>
    ${stb_image_SOURCE_DIR}
    ${THIRD_PARTY_INSTALL}/include)

link_directories(${THIRD_PARTY_INSTALL}/lib)

add_dependencies(heic_demo heif_static x265_static)

target_link_libraries(heic_demo PRIVATE
    glfw
    heif_static
    x265_static
    opengl32)

# Produce a single stand‑alone binary
if(MINGW)
  target_link_options(heic_demo PRIVATE -static -static-libgcc -static-libstdc++)
endif()