add_library(circsmp
    Utils.cpp
    Party.cpp
    PermutationNetwork.cpp
    Sort/SortingNetwork.cpp
    Sort/OptSortingNetwork.cpp
    Sort/ShuffleQSort.cpp
    Sort/YaoSortingNetwork.cpp
)

target_link_libraries(circsmp INTERFACE
    ABY::aby
    ENCRYPTO_utils::encrypto_utils
    relic_s
)

target_include_directories(circsmp PUBLIC
    ../../extern/ABY/src/abycore
    ../../extern/ABY/extern/ENCRYPTO_utils/src
    ${PROJECT_BINARY_DIR}/extern/ABY/extern/ENCRYPTO_utils/include/
)
