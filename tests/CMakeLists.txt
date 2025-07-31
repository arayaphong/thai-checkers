# Create test executable for Catch2
add_executable(BoardTest
    BoardTest.cpp
)

add_executable(PositionTest
    PositionTest.cpp
)

add_executable(DameAnalyzerTest
    DameAnalyzerTest.cpp
)

add_executable(PionAnalyzerTest
    PionAnalyzerTest.cpp
)

add_executable(MoveTest
    MoveTest.cpp
)

# Include directories for the test executables
target_include_directories(BoardTest
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../include
)

target_include_directories(PositionTest
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../include
)

target_include_directories(DameAnalyzerTest
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../include
)

target_include_directories(PionAnalyzerTest
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../include
)

target_include_directories(MoveTest
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../include
)

# Link with project library and Catch2
target_link_libraries(BoardTest
    PRIVATE
    thai_checkers_lib
    Catch2::Catch2WithMain
)

target_link_libraries(PositionTest
    PRIVATE
    thai_checkers_lib
    Catch2::Catch2WithMain
)

target_link_libraries(DameAnalyzerTest
    PRIVATE
    thai_checkers_lib
    Catch2::Catch2WithMain
)

target_link_libraries(PionAnalyzerTest
    PRIVATE
    thai_checkers_lib
    Catch2::Catch2WithMain
)

target_link_libraries(MoveTest
    PRIVATE
    thai_checkers_lib
    Catch2::Catch2WithMain
)

# Register the test with CTest
add_test(NAME BoardTest COMMAND BoardTest)
add_test(NAME PositionTest COMMAND PositionTest)
add_test(NAME DameAnalyzerTest COMMAND DameAnalyzerTest)
add_test(NAME PionAnalyzerTest COMMAND PionAnalyzerTest)
add_test(NAME MoveTest COMMAND MoveTest)
