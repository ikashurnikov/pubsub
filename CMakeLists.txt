project(pubsub)
set(CMAKE_CXX_STANDARD 17)

set(sources 
    data.h
    func_traits.h
    runtime.h
    runtime.cpp 
    channel.h
    example.cpp)

add_executable(example ${sources})
