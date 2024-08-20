cmake_minimum_required(VERSION 3.21)

project(template-library)

if (PROJECT_IS_TOP_LEVEL)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    	
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin-lib")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin-etc")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
    
    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_CXX_STANDARD_REQUIRED)

    set(FOX_TEMPLATE_LIBRARY_BUILD_SAMPLES ON CACHE BOOL "")
    set(FOX_TEMPLATE_LIBRARY_BUILD_TESTS ON CACHE BOOL "")
endif()

option(FOX_TEMPLATE_LIBRARY_BUILD_SAMPLES "If samples are built." OFF)
option(FOX_TEMPLATE_LIBRARY_BUILD_TESTS "If unit tests are built" OFF)
    
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include(FetchContent)
add_subdirectory("include")
add_library(fox::template_library ALIAS fox-template-library)

if (FOX_TEMPLATE_LIBRARY_BUILD_SAMPLES)
	add_subdirectory("sample")
endif()

if (FOX_TEMPLATE_LIBRARY_BUILD_TESTS)
	enable_testing()
	add_subdirectory("test")
endif()