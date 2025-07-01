# Cross‑compile to 64‑bit Windows from Linux
set(CMAKE_SYSTEM_NAME Windows)          # already present
set(CMAKE_SYSTEM_PROCESSOR x86_64)      # <-- ADD THIS LINE

set(CMAKE_C_COMPILER   x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER  x86_64-w64-mingw32-windres)

# Where MinGW keeps its headers & libs
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)

# Prefer the target root for headers and libs
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)