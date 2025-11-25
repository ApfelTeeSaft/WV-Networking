cmake_minimum_required(VERSION 3.24)

add_executable(SimpleServer
    main.cpp
)

target_link_libraries(SimpleServer PRIVATE WVNet)

target_compile_features(SimpleServer PRIVATE cxx_std_20)
