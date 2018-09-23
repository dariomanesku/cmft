add_subdirectory(cmft)

add_executable(cmft_cli
        main.cpp)

IF(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
target_link_libraries(cmft_cli
        cmft -lpthread)
ELSE()
target_link_libraries(cmft_cli
        cmft -ldl -lpthread)
ENDIF()

install(TARGETS cmft_cli
    RUNTIME DESTINATION bin
)
