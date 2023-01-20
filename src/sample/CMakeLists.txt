add_executable(offline
    demo.cpp
    shufflesample.cpp
    wrsample.cpp
    worsample.cpp
    stratifiedsample.cpp
)

target_compile_options(offline PRIVATE "-Wall" "-Wextra" "-O2")
target_link_libraries(offline 
    PUBLIC circsmp
    PUBLIC ABY::aby
    PUBLIC ENCRYPTO_utils::encrypto_utils
)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})