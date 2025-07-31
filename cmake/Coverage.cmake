# Coverage.cmake - Coverage configuration for Thai Checkers project

# Function to setup coverage
function(setup_coverage)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        # Coverage flags
        set(COVERAGE_FLAGS "--coverage -fprofile-arcs -ftest-coverage")
        set(COVERAGE_LIBS "--coverage")
        
        # Add coverage flags to all targets
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COVERAGE_FLAGS}" PARENT_SCOPE)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${COVERAGE_LIBS}" PARENT_SCOPE)
        
        message(STATUS "Coverage enabled for ${CMAKE_CXX_COMPILER_ID}")
    else()
        message(WARNING "Coverage not supported for compiler: ${CMAKE_CXX_COMPILER_ID}")
    endif()
endfunction()

# Function to add coverage target
function(add_coverage_target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        # Find required tools
        find_program(GCOV_PATH gcov)
        find_program(LCOV_PATH lcov)
        find_program(GENHTML_PATH genhtml)
        
        if(GCOV_PATH AND LCOV_PATH AND GENHTML_PATH)
            # Create coverage directory
            set(COVERAGE_DIR "${CMAKE_BINARY_DIR}/coverage")
            file(MAKE_DIRECTORY ${COVERAGE_DIR})
            
            # Add custom target for coverage
            add_custom_target(coverage
                # Clean previous coverage data
                COMMAND ${LCOV_PATH} --directory . --zerocounters
                
                # Run tests
                COMMAND ${CMAKE_CTEST_COMMAND} --verbose
                
                # Capture coverage data
                COMMAND ${LCOV_PATH} --directory . --capture --output-file ${COVERAGE_DIR}/coverage.info
                
                # Remove unwanted files (external libraries, tests)
                COMMAND ${LCOV_PATH} --remove ${COVERAGE_DIR}/coverage.info 
                    '/usr/*' 
                    '*/tests/*' 
                    '*/catch2/*'
                    --output-file ${COVERAGE_DIR}/coverage_cleaned.info
                
                # Generate HTML report
                COMMAND ${GENHTML_PATH} ${COVERAGE_DIR}/coverage_cleaned.info 
                    --output-directory ${COVERAGE_DIR}/html
                    --title "Thai Checkers Coverage Report"
                    --show-details
                    --legend
                
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Generating coverage report"
                VERBATIM
            )
            
            message(STATUS "Coverage target 'coverage' added")
            message(STATUS "Run 'make coverage' or 'cmake --build . --target coverage' to generate report")
        else()
            message(WARNING "Coverage tools not found. Install lcov and genhtml.")
        endif()
    endif()
endfunction()
