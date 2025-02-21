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


#include <tracy/Tracy.hpp>

#include <vector>
#include <iostream>
#include <random>
#include <thread>
#include <atomic>
#include <ranges>

#include <lock-free-bounded-queue/lock-free-bounded-queue.hpp>

#if defined (USE_THREAD_YIELD)
    #define thread_yield() std::this_thread::yield()
#else 
    #define thread_yield()
#endif

constexpr static std::size_t QUEUE_SIZE{ 1 << 10 };
constexpr static std::size_t TASK_COUNT{ 100'000 }; // The number of tasks must be divided by the number of producers 
constexpr static std::size_t RANDOM_BUFFER_SIZE{ 2'048 };

using LFQueue_ = LFQueue<QUEUE_SIZE>;
using SortBuffer_ = std::vector<std::ptrdiff_t>;

using FutureBuffer_ = std::vector<std::future<SortBuffer_>>;
using DoneFlag_ = std::atomic<bool>;

void* operator new(std::size_t count)
{
    auto p{ std::malloc(count) };
    TracyAlloc(p, count);

    return p;
}

void operator delete(void* p) noexcept
{
    TracyFree(p);
    free(p);
}

// Fill buffer with random numbers
SortBuffer_ FillRandomBuffer()
{
    static std::random_device random_device{};
    static std::mt19937 mt{ random_device() };

    static std::uniform_int_distribution<std::ptrdiff_t> dist{ 0, 100'000 };

    auto gen = []() -> std::ptrdiff_t
    {
        return dist(mt);
    };

    SortBuffer_ buffer{};
    buffer.resize(RANDOM_BUFFER_SIZE);

    std::ranges::generate(buffer, gen);
    return buffer;
}

// Our task with time complexity O(N^2 + N)
SortBuffer_ BubbleSort()
{
    auto buffer{ FillRandomBuffer() };
    auto buf_size{ static_cast<std::ptrdiff_t>(std::size(buffer)) };

    for (std::ptrdiff_t i{}; i < buf_size; ++i)
    {
        for (std::ptrdiff_t j{}; j < buf_size - i - 1; ++j)
        {
            if (buffer[j] > buffer[j + 1])
            {
                std::swap(buffer[j], buffer[j + 1]);
            }
        }
    }

    return buffer;
}

void Producer(LFQueue_& queue, FutureBuffer_& future_buffer, std::mutex& mutex, const std::ptrdiff_t task_count)
{
    ZoneScopedNC(__FUNCTION__, tracy::Color::Yellow);

    FutureBuffer_ local_future_buffer{};
    local_future_buffer.reserve(task_count);

    for (std::ptrdiff_t i{}; i < task_count; ++i)
    {
        auto&& [task, future] { abstract_task::CreateTask(BubbleSort) };

        while (!queue.TryPush(task))
        {
            thread_yield();
        }

        local_future_buffer.emplace_back(std::move(future));
    }

    std::lock_guard<std::mutex> lk{ mutex };
    std::ranges::move(local_future_buffer, std::back_inserter(future_buffer));
}

void Consumer(LFQueue_& queue, DoneFlag_& is_done)
{
    ZoneScopedNC(__FUNCTION__, tracy::Color::Cyan);

    while (!is_done.load(std::memory_order_acquire) || !queue.IsEmpty())
    {
        LFQueue_::abstract_task_t task{};
        if (queue.TryPop(task))
        {
            if (task() != 0)
            {
                std::cerr << "Error: task() != 0, thread_id: " << std::this_thread::get_id() << '\n';
                return;
            }

            continue;
        }

        thread_yield();
    }
}

void StartProfiling(const std::size_t consumer_threads_count, const std::size_t producer_threads_count)
{
    ZoneScopedNC(__FUNCTION__, tracy::Color::Green);

    LFQueue_ queue{};
    DoneFlag_ is_done{ false };

    std::mutex mutex{};
    const auto task_count{ TASK_COUNT / producer_threads_count };

    SortBuffer_ result_buffer{};
    result_buffer.reserve(TASK_COUNT);

    FutureBuffer_ future_buffer{};
    future_buffer.reserve(TASK_COUNT);

    std::vector<std::thread> consumer_threads{};
    consumer_threads.reserve(consumer_threads_count);

    std::vector<std::thread> producer_threads{};
    producer_threads.reserve(producer_threads_count);

    for (std::size_t i{}; i < consumer_threads_count; ++i)
    {
        consumer_threads.emplace_back(Consumer, std::ref(queue), std::ref(is_done));
    }

    for (std::size_t i{}; i < producer_threads_count; ++i)
    {
        producer_threads.emplace_back(Producer, std::ref(queue), std::ref(future_buffer), std::ref(mutex), task_count);
    }

    for (auto&& p : producer_threads)
    {
        p.join();
    }

    is_done.store(true, std::memory_order_release);

    for (auto&& c : consumer_threads)
    {
        c.join();
    }

    for (auto&& f : future_buffer)
    {
        std::ranges::copy(f.get(), std::back_inserter(result_buffer));
    }

#if defined (SHOW_RESULTS)
    for (std::size_t i{}; i < TASK_COUNT; ++i)
    {
        for (std::size_t j{}; j < RANDOM_BUFFER_SIZE; ++j)
        {
            std::cout << result_buffer[i * RANDOM_BUFFER_SIZE + j] << " ";
        }

        std::cout << "\n\n";
    }
#endif
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv)
{
    ZoneScopedNC(__FUNCTION__, tracy::Color::Red);
    StartProfiling(16, 16);
    
    return 0;
}