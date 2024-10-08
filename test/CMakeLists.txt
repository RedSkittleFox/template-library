cmake_minimum_required(VERSION 3.5)

cmake_policy(PUSH)

enable_testing()

if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
    cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.13.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(sources 
    "${CMAKE_CURRENT_SOURCE_DIR}/iterator/indirect_iterator_test.cc"

    "${CMAKE_CURRENT_SOURCE_DIR}/inplace_free_list_test.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/free_list_test.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/ptr_vector_test.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/intrusive_list_test.cc"
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})

add_executable(
    fox-template-library-test
    ${sources}
)

if(MSVC)
	target_compile_options(
	    fox-template-library-test
		PRIVATE /W4 
		PRIVATE /MP 
		PRIVATE /arch:AVX2
		PRIVATE /WX
	)

endif()

target_link_libraries(
    fox-template-library-test    
    GTest::gtest_main
    GTest::gmock_main
    fox-template-library
)

include(GoogleTest)
gtest_discover_tests(fox-template-library-test)

if (PROJECT_IS_TOP_LEVEL)
    set_target_properties(gtest_main PROPERTIES FOLDER "vendor")
    set_target_properties(gtest PROPERTIES FOLDER "vendor")
    set_target_properties(gmock_main PROPERTIES FOLDER "vendor")
    set_target_properties(gmock PROPERTIES FOLDER "vendor")
endif()

cmake_policy(POP)