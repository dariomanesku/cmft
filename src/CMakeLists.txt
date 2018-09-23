add_subdirectory(cmft)

add_executable(cmft_cli
        main.cpp)

target_link_libraries(cmft_cli
        cmft)

install(TARGETS cmft_cli
    RUNTIME DESTINATION bin
)