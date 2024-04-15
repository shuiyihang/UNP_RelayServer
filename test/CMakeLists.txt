add_executable(trigger
    trigger.cpp
    generator.cpp
    generator.h
)
target_link_libraries(trigger PRIVATE unpWork)

add_executable(server
    server.cpp
)

target_link_libraries(server PRIVATE unpWork)


add_executable(client
    client.cpp
)
target_link_libraries(client PRIVATE unpWork)