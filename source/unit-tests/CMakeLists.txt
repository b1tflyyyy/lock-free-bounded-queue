# MIT License
# 
# Copyright (c) 2025 @Who
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.


cmake_minimum_required(VERSION 3.22)

project(tests LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_LEAK_SANITIZER OFF)
option(ENABLE_THREAD_SANITIZER OFF)

set(SANITIZER)

if (ENABLE_LEAK_SANITIZER)
    set(SANITIZER -fsanitize=leak,undefined,address)
endif()

if (ENABLE_THREAD_SANITIZER)
    set(SANITIZER -fsanitize=thread)
endif()

add_compile_options(${SANITIZER})

file(GLOB_RECURSE TESTS ${CMAKE_CURRENT_SOURCE_DIR}/*pass.cpp)

add_executable(${PROJECT_NAME} ${TESTS})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/source)
target_link_libraries(${PROJECT_NAME} PRIVATE GTest::gtest ${SANITIZER})