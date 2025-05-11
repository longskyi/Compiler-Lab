cmake_minimum_required(VERSION 3.16.0)
project(LCCompiler VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)


set(SOURCE 
    ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp)

add_executable(${PROJECT_NAME} ${SOURCE})

set_target_properties(${PROJECT_NAME} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_LIST_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_LIST_DIR}/bin
)

if (MSVC)
    target_compile_options(${PROJECT_NAME}
        PRIVATE /utf-8
    )
    target_compile_definitions(${PROJECT_NAME}
        PRIVATE NOMINMAX
    )
else()
    target_compile_options(${PROJECT_NAME}
        PRIVATE -Wall -Wextra -Werror -fuse-ld=lld
    )
endif()
