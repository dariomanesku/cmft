add_subdirectory(common)

add_library(cmft allocator.cpp clcontext.cpp cubemapfilter.cpp image.cpp)

target_link_libraries(cmft common)

