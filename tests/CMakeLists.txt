# Source 1.5 Test Suite

# Fetch Catch2 from GitHub
include(FetchContent)
FetchContent_Declare(
	Catch2
	GIT_REPOSITORY https://github.com/catchorg/Catch2.git
	GIT_TAG v3.5.1
)
FetchContent_MakeAvailable(Catch2)

# Test executable
add_executable(source15_tests
	# Framework tests
	test_string_utils.cpp
	test_string_utils_advanced.cpp
	test_color_grading.cpp

	# Add more test files here
)

target_link_libraries(source15_tests PRIVATE
	Catch2::Catch2WithMain
	lib_framework
)

target_include_directories(source15_tests PRIVATE
	${CMAKE_SOURCE_DIR}/src/framework
	${CMAKE_SOURCE_DIR}/src/public
)

# Register tests with CTest
include(CTest)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(source15_tests)

# Add custom target to run tests
add_custom_target(run_tests
	COMMAND source15_tests
	DEPENDS source15_tests
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	COMMENT "Running Source 1.5 unit tests"
)
