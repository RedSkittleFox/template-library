cmake_minimum_required(VERSION 3.21)

set(sources 
    "${CMAKE_CURRENT_SOURCE_DIR}/fox/iterator/indirect_iterator.hpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/fox/inplace_free_list.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/fox/free_list.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/fox/ptr_vector.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/fox/intrusive_list.hpp"
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})

add_library(
    fox-template-library
    INTERFACE
    ${sources}
)

target_include_directories(
    fox-template-library
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
