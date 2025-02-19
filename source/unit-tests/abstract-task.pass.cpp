// MIT License
// 
// Copyright (c) 2025 @Who
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


#include <gtest/gtest.h>

#include <abstract-task/abstract-task.hpp>

int add(int a, int b)
{
    return a + b;
}

TEST(AbstractTask, move_default_func)
{
    {
        auto&& [task_1, future_1]{ abstract_task::CreateTask(add, 1, 2) };
        auto task_2{ std::move(task_1) };
        
        EXPECT_EQ(task_2(), 0);
        EXPECT_EQ(future_1.get(), 3);
    }
}

TEST(AbstractTask, move_lambda)
{
    {
        auto&& [task_1, future_1]{ abstract_task::CreateTask([](int a, int b, int c) -> int { return a + b + c; }, 1, 2, 3) };
        auto task_2{ std::move(task_1) };
        
        EXPECT_EQ(task_2(), 0);
        EXPECT_EQ(future_1.get(), 6);
    }
}