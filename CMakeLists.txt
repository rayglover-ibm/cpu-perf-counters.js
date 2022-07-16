cmake_minimum_required (VERSION 3.11)
include (FetchContent)

project (cpu-perf-counters)

set (CMAKE_POSITION_INDEPENDENT_CODE ON)

# Essential include files to build a node addon with cmake.js
include_directories (${CMAKE_JS_INC})

add_library (${PROJECT_NAME} SHARED
    "src/addon.cc"

    # Linux only
    "$<IF:$<PLATFORM_ID:Linux>,"
        "src/counter_group.linux.cc,"
        "src/counter_group.unknown.cc"
    ">"
)

set_target_properties (${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    PREFIX ""
    SUFFIX ".node"
)

# Find the N-API wrappers
execute_process (
    COMMAND node -e "process.stdout.write(require('node-addon-api').include.replace(/^\"|\"+$/g, ''))"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE NODE_ADDON_API_DIR
)

# Include N-API wrappers
target_include_directories (${PROJECT_NAME}
    PRIVATE "${NODE_ADDON_API_DIR}"
)

# N-API v6 required for BigInt support
target_compile_definitions (${PROJECT_NAME}
    PUBLIC NAPI_VERSION=6
)

target_compile_options (${PROJECT_NAME} PUBLIC
    "$<$<CXX_COMPILER_ID:Clang>:-Wall;-Wextra;-pedantic;-msse2;-O2;-fno-omit-frame-pointer>"
)

# Essential library files to link to a node addon,
# you should add this line in every CMake.js based project.
target_link_libraries (${PROJECT_NAME} ${CMAKE_JS_LIB})
