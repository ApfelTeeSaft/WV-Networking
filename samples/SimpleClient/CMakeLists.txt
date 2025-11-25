cmake_minimum_required(VERSION 3.24)

add_executable(SimpleClient
    main.cpp
)

target_link_libraries(SimpleClient PRIVATE WVNet)

target_compile_features(SimpleClient PRIVATE cxx_std_20)
