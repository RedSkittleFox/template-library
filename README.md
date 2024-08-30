[![CMake CLANG Windows Build and Test](https://github.com/RedSkittleFox/template-library/actions/workflows/cmake-clang-windows.yml/badge.svg)](https://github.com/RedSkittleFox/template-library/actions/workflows/cmake-clang-windows.yml)
[![CMake MSVC Windows Build and Test](https://github.com/RedSkittleFox/template-library/actions/workflows/cmake-msvc-windows.yml/badge.svg)](https://github.com/RedSkittleFox/template-library/actions/workflows/cmake-msvc-windows.yml)

# template-library
Standard template library extension.

# Modules

- [fox::iterator](/include/fox/iterator) - additional iterator adaptors
- [fox::free_list](/include/fox/free_list.hpp) - free-list implementation
- [fox::inplace_free_list](/include/fox/inplace_free_list.hpp) - inplace free-list implementation
- [fox::ptr_vector](/include/fox/ptr_vector.hpp) - `std::vector` like data structure but elements are stored indirectly

# Supported compilers

This is a C++ 23 library.

- Microsoft Visual C++ 2022 (msvc) / Build Tools 19.38.33134 (and possibly later)
- Microsoft Visual C++ 2022 (clang) 16.0.5 (and possibly later)

# Integration

```cmake
include(FetchContent)

FetchContent_Declare(fox_template_library 
    GIT_REPOSITORY https://github.com/RedSkittleFox/template-library.git
    GIT_TAG main
)

FetchContent_MakeAvailable(fox_template_library)
target_link_libraries(foo PRIVATE fox::template_library)
```

# License
This library is licensed under the [MIT License](LICENSE).
