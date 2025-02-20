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

#include <vector>
#include <mutex>
#include <ranges>
#include <format>

#include <lock-free-bounded-queue/lock-free-bounded-queue.hpp>

constexpr static std::size_t TASK_COUNT{ 100'000 };
constexpr static std::size_t QUEUE_SIZE{ 1 << 10 };

using LFQueue_ = LFQueue<QUEUE_SIZE>;
using TaskFutureBuffer_ = std::vector<std::future<std::size_t>>;

void print_result_buffer(const std::vector<std::size_t>& result_buffer)
{
    for (std::size_t i{}; i < std::size(result_buffer); ++i)
    {
        std::cout << std::format("{} + {} + {} =\t{}\n", i + 1, i + 2, i + 3, result_buffer[i]);
    }
}

void consume(LFQueue_& queue, std::atomic<bool>& is_done)
{
    while (!is_done.load(std::memory_order_acquire) || !queue.IsEmpty())
    {
        LFQueue_::abstract_task_t task{};
        if (queue.TryPop(task))
        {
            auto result{ task() };
            if (result != 0)
            {
                std::cerr << "result != 0\n";
                return;
            }

            continue;
        }

        std::this_thread::yield();
    }
}

void produce(LFQueue_& queue, TaskFutureBuffer_& task_future_buffer, std::mutex& mutex, const std::size_t task_count)
{
    TaskFutureBuffer_ local_task_future_buffer{};
    local_task_future_buffer.reserve(task_count);

    for (std::size_t i{}; i < task_count; ++i)
    {
        auto&& [task, future] { abstract_task::CreateTask([](std::size_t a, std::size_t b, std::size_t c) -> std::size_t 
        {
            return a + b + c;
        }, i + 1, i + 2, i + 3) };

        while (!queue.TryPush(task))
        {
            std::this_thread::yield();
        }

        local_task_future_buffer.push_back(std::move(future));
    }

    std::lock_guard<std::mutex> lk{ mutex };
    std::ranges::move(local_task_future_buffer, std::back_inserter(task_future_buffer));
}


// test_1c_1p -> 1 producer -> 1 consumer
TEST(LockFreeBoundedQueue, test_1c_1p)
{
    LFQueue_ queue{};
    
    TaskFutureBuffer_ task_future_buffer{};
    task_future_buffer.reserve(TASK_COUNT);

    std::vector<std::size_t> result_buffer{};
    result_buffer.reserve(TASK_COUNT);

    std::atomic<bool> is_done{ false };
    std::mutex mutex{};

    std::thread consumer{ consume, std::ref(queue), std::ref(is_done) };
    std::thread producer{ produce, std::ref(queue), std::ref(task_future_buffer), std::ref(mutex), TASK_COUNT };

    producer.join();
    is_done.store(true, std::memory_order_release);
    consumer.join();
    
    for (auto&& f : task_future_buffer)
    {
        result_buffer.push_back(f.get());
    }

#if defined(PRINT_RES_BUF)
    print_result_buffer(result_buffer);
#endif

    ASSERT_EQ(std::size(task_future_buffer), TASK_COUNT);
    ASSERT_EQ(std::size(result_buffer), TASK_COUNT);
}

// test_2c_2p -> 2 producer -> 2 consumer
TEST(LockFreeBoundedQueue, test_2c_2p)
{
    LFQueue_ queue{};
    
    TaskFutureBuffer_ task_future_buffer{};
    task_future_buffer.reserve(TASK_COUNT);

    std::vector<std::size_t> result_buffer{};
    result_buffer.reserve(TASK_COUNT);

    std::atomic<bool> is_done{ false };
    std::mutex mutex{};

    std::vector<std::thread> consumers{};
    std::vector<std::thread> producers{};

    for (std::size_t i{}; i < 2; ++i)
    {
        consumers.emplace_back(consume, std::ref(queue), std::ref(is_done));
        producers.emplace_back(produce, std::ref(queue), std::ref(task_future_buffer), std::ref(mutex), TASK_COUNT / 2);
    }

    for (auto&& p : producers)
    {
        p.join();
    }

    is_done.store(true, std::memory_order_release);

    for (auto&& c : consumers)
    {
        c.join();
    }
    
    for (auto&& f : task_future_buffer)
    {
        result_buffer.push_back(f.get());
    }

#if defined(PRINT_RES_BUF)
    print_result_buffer(result_buffer);
#endif

    ASSERT_EQ(std::size(task_future_buffer), TASK_COUNT);
    ASSERT_EQ(std::size(result_buffer), TASK_COUNT);
}

// test_4c_4p -> 4 producer -> 4 consumer
TEST(LockFreeBoundedQueue, test_4c_4p)
{
    LFQueue_ queue{};
    
    TaskFutureBuffer_ task_future_buffer{};
    task_future_buffer.reserve(TASK_COUNT);

    std::vector<std::size_t> result_buffer{};
    result_buffer.reserve(TASK_COUNT);

    std::atomic<bool> is_done{ false };
    std::mutex mutex{};

    std::vector<std::thread> consumers{};
    std::vector<std::thread> producers{};

    for (std::size_t i{}; i < 4; ++i)
    {
        consumers.emplace_back(consume, std::ref(queue), std::ref(is_done));
        producers.emplace_back(produce, std::ref(queue), std::ref(task_future_buffer), std::ref(mutex), TASK_COUNT / 4);
    }

    for (auto&& p : producers)
    {
        p.join();
    }

    is_done.store(true, std::memory_order_release);

    for (auto&& c : consumers)
    {
        c.join();
    }
    
    for (auto&& f : task_future_buffer)
    {
        result_buffer.push_back(f.get());
    }

#if defined(PRINT_RES_BUF)
    print_result_buffer(result_buffer);
#endif

    ASSERT_EQ(std::size(task_future_buffer), TASK_COUNT);
    ASSERT_EQ(std::size(result_buffer), TASK_COUNT);
}

// test_8c_8p -> 8 producer -> 8 consumer
TEST(LockFreeBoundedQueue, test_8c_8p)
{
    LFQueue_ queue{};
    
    TaskFutureBuffer_ task_future_buffer{};
    task_future_buffer.reserve(TASK_COUNT);

    std::vector<std::size_t> result_buffer{};
    result_buffer.reserve(TASK_COUNT);

    std::atomic<bool> is_done{ false };
    std::mutex mutex{};

    std::vector<std::thread> consumers{};
    std::vector<std::thread> producers{};

    for (std::size_t i{}; i < 8; ++i)
    {
        consumers.emplace_back(consume, std::ref(queue), std::ref(is_done));
        producers.emplace_back(produce, std::ref(queue), std::ref(task_future_buffer), std::ref(mutex), TASK_COUNT / 8);
    }

    for (auto&& p : producers)
    {
        p.join();
    }

    is_done.store(true, std::memory_order_release);

    for (auto&& c : consumers)
    {
        c.join();
    }
    
    for (auto&& f : task_future_buffer)
    {
        result_buffer.push_back(f.get());
    }

#if defined(PRINT_RES_BUF)
    print_result_buffer(result_buffer);
#endif

    ASSERT_EQ(std::size(task_future_buffer), TASK_COUNT);
    ASSERT_EQ(std::size(result_buffer), TASK_COUNT);
}

// test_10c_10p -> 10 producer -> 10 consumer
TEST(LockFreeBoundedQueue, test_10c_10p)
{
    LFQueue_ queue{};
    
    TaskFutureBuffer_ task_future_buffer{};
    task_future_buffer.reserve(TASK_COUNT);

    std::vector<std::size_t> result_buffer{};
    result_buffer.reserve(TASK_COUNT);

    std::atomic<bool> is_done{ false };
    std::mutex mutex{};

    std::vector<std::thread> consumers{};
    std::vector<std::thread> producers{};

    for (std::size_t i{}; i < 10; ++i)
    {
        consumers.emplace_back(consume, std::ref(queue), std::ref(is_done));
        producers.emplace_back(produce, std::ref(queue), std::ref(task_future_buffer), std::ref(mutex), TASK_COUNT / 10);
    }

    for (auto&& p : producers)
    {
        p.join();
    }

    is_done.store(true, std::memory_order_release);

    for (auto&& c : consumers)
    {
        c.join();
    }
    
    for (auto&& f : task_future_buffer)
    {
        result_buffer.push_back(f.get());
    }

#if defined(PRINT_RES_BUF)
    print_result_buffer(result_buffer);
#endif

    ASSERT_EQ(std::size(task_future_buffer), TASK_COUNT);
    ASSERT_EQ(std::size(result_buffer), TASK_COUNT);
}

// test_16c_16p -> 16 producer -> 16 consumer
TEST(LockFreeBoundedQueue, test_16c_16p)
{
    LFQueue_ queue{};
    
    TaskFutureBuffer_ task_future_buffer{};
    task_future_buffer.reserve(TASK_COUNT);

    std::vector<std::size_t> result_buffer{};
    result_buffer.reserve(TASK_COUNT);

    std::atomic<bool> is_done{ false };
    std::mutex mutex{};

    std::vector<std::thread> consumers{};
    std::vector<std::thread> producers{};

    for (std::size_t i{}; i < 16; ++i)
    {
        consumers.emplace_back(consume, std::ref(queue), std::ref(is_done));
        producers.emplace_back(produce, std::ref(queue), std::ref(task_future_buffer), std::ref(mutex), TASK_COUNT / 16);
    }

    for (auto&& p : producers)
    {
        p.join();
    }

    is_done.store(true, std::memory_order_release);

    for (auto&& c : consumers)
    {
        c.join();
    }
    
    for (auto&& f : task_future_buffer)
    {
        result_buffer.push_back(f.get());
    }

#if defined(PRINT_RES_BUF)
    print_result_buffer(result_buffer);
#endif

    ASSERT_EQ(std::size(task_future_buffer), TASK_COUNT);
    ASSERT_EQ(std::size(result_buffer), TASK_COUNT);
}