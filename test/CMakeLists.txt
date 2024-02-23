add_executable(client
    client.cpp
)
target_link_libraries(client PRIVATE unpWork)

add_executable(server
    server.cpp
)

target_link_libraries(server PRIVATE unpWork)