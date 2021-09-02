project(pubsub)
set(CMAKE_CXX_STANDARD 17)

set(sources 
    data.h
    func_traits.h
    runtime.h
    runtime.cpp 
    channel.h
    example.cpp)

find_package(Threads REQUIRED)

add_executable(example ${sources})
target_link_libraries(example Threads::Threads)
