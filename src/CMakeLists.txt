if(MSVC)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D _CRT_SECURE_NO_WARNINGS")
endif(MSVC)

add_subdirectory(cmft)

add_executable(cmft_cli
        main.cpp)

IF(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
target_link_libraries(cmft_cli
        cmft -lpthread)
ELSEIF(${WIN32})
target_link_libraries(cmft_cli
        cmft)
ELSE()
target_link_libraries(cmft_cli
        cmft -ldl -lpthread)
ENDIF()

install(TARGETS cmft_cli
    RUNTIME DESTINATION bin
)
