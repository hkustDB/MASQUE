add_executable(online
    tpchquery.cpp
)

target_compile_options(online PRIVATE "-Wall" "-Wextra" "-O2")
target_link_libraries(online 
    PUBLIC circsmp
    PUBLIC ABY::aby
    PUBLIC ENCRYPTO_utils::encrypto_utils
)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})