add_executable(generator
    generator.cpp
)
target_link_libraries(generator PRIVATE unpWork)

add_executable(server
    server.cpp
)

target_link_libraries(server PRIVATE unpWork)