cmake_minimum_required(VERSION 3.16.0)
project(LCCompiler VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(ASTROOT
    ${CMAKE_CURRENT_LIST_DIR}/src/AST
)

set(ASTSOURCE
    ${ASTROOT}/AST.cpp
    ${ASTROOT}/NodeType/Program.cpp
    ${ASTROOT}/NodeType/ASTbaseType.cpp
    ${ASTROOT}/NodeType/Declare.cpp
    ${ASTROOT}/NodeType/Param.cpp
    ${ASTROOT}/NodeType/Expr.cpp
    ${ASTROOT}/NodeType/Stmt.cpp
)

set(SOURCE 
    ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp
    # ${CMAKE_CURRENT_LIST_DIR}/src/mylex.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/lexer.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/stringUtil.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/SyntaxType.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/fileIO.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/parserGen.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/Semantic.cpp
    ${ASTSOURCE}
    )


add_executable(${PROJECT_NAME} ${SOURCE})
target_include_directories(${PROJECT_NAME}
    PRIVATE    ${CMAKE_CURRENT_LIST_DIR}/include
)
target_include_directories(${PROJECT_NAME}
    PRIVATE    ${CMAKE_CURRENT_LIST_DIR}/deps
)
set_target_properties(${PROJECT_NAME} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_LIST_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_LIST_DIR}/bin
)

if (MSVC)
    target_compile_options(${PROJECT_NAME}
        PRIVATE /utf-8 /std:c++latest /MP
    )
    target_compile_definitions(${PROJECT_NAME}
        PRIVATE NOMINMAX
    )
else()
    target_compile_options(${PROJECT_NAME}
        PRIVATE -Werror
    )
endif()
