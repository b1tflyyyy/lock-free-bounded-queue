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


#pragma once

#include <future>
#include <tuple>
#include <functional>
#include <utility>

namespace abstract_task 
{
    template <typename T>
    class Task;

    template <typename ReturnType, typename... Args>
    class Task<ReturnType(Args...)>
    {
    public:
        template <typename FunctionType>
        explicit Task(FunctionType&& function)
        {
            mInternalFunction = reinterpret_cast<void*>(new FunctionType{ std::move(function) });
        
            mCleaner = [](void* function) -> void
            {
                auto* typed_function_ptr{ reinterpret_cast<FunctionType*>(function) };
                delete typed_function_ptr;
            };

            mInvoke = [](void* function, Args&&... args) -> ReturnType
            {
                auto typed_function_ptr{ reinterpret_cast<FunctionType*>(function) };
                return (*typed_function_ptr)(std::forward<Args>(args)...);
            };
        }

        ~Task() noexcept 
        {
            if (mCleaner)
            {
                mCleaner(mInternalFunction);
            }
        }

        Task(const Task& other) = delete;
        Task(Task&& other) noexcept
        {
            mInternalFunction = other.mInternalFunction;
            other.mInternalFunction = nullptr;

            mCleaner = std::move(other.mCleaner);
            mInvoke = std::move(other.mInvoke);
        }

        Task& operator=(const Task& other) = delete;
        [[nodiscard]] Task& operator=(Task&& other) noexcept
        {
            mInternalFunction = other.mInternalFunction;
            other.mInternalFunction = nullptr;

            mCleaner = std::move(other.mCleaner);
            mInvoke = std::move(other.mInvoke);

            return *this;
        }

        [[nodiscard]] ReturnType operator()(Args&&... args)
        {
            return mInvoke(mInternalFunction, std::forward<Args>(args)...);
        }

    private:
        void* mInternalFunction;

        std::function<void(void*)> mCleaner;
        std::function<ReturnType(void*, Args&&...)> mInvoke;
    };

    template <typename FunctionType, typename... Args>
    [[nodiscard]] auto CreateTask(FunctionType&& function, Args&&... args)
    {
        std::packaged_task<typename std::invoke_result_t<FunctionType, Args...>(Args...)> packaged_task{ std::move(function) };
        auto future{ packaged_task.get_future() };

        Task<std::int32_t()> abstract_task
        {
            [m_func = std::move(packaged_task), args = std::make_tuple(std::forward<Args>(args)...)]() mutable -> std::int32_t
            {
                std::apply([m_func = std::move(m_func)](auto&&... args) mutable -> void
                {
                    m_func(args...);
                }, std::move(args));

                return 0;
            }
        };

        return std::make_pair(std::move(abstract_task), std::move(future));
    }
}