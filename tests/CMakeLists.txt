# Create test executable for Catch2
add_executable(BoardTest
    BoardTest.cpp
)

add_executable(PositionTest
    PositionTest.cpp
)

add_executable(AnalyzerTest
    AnalyzerTest.cpp
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

target_include_directories(AnalyzerTest
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

target_link_libraries(AnalyzerTest
    PRIVATE
    thai_checkers_lib
    Catch2::Catch2WithMain
)


# Register the test with CTest
add_test(NAME BoardTest COMMAND BoardTest)
add_test(NAME PositionTest COMMAND PositionTest)
add_test(NAME AnalyzerTest COMMAND AnalyzerTest)
