# ThreadPool
add_library(pool INTERFACE)
target_include_directories(pool SYSTEM INTERFACE ThreadPool)

# json
set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(json)

# stb
add_library(stb INTERFACE)
target_compile_definitions(stb INTERFACE STB_IMAGE_IMPLEMENTATION)
target_include_directories(stb SYSTEM INTERFACE stb)

# tinyexr
add_library(tinyexr INTERFACE)
target_compile_definitions(tinyexr INTERFACE TINYEXR_IMPLEMENTATION)
target_include_directories(tinyexr SYSTEM INTERFACE tinyexr)

# pybind11
add_subdirectory(pybind11)