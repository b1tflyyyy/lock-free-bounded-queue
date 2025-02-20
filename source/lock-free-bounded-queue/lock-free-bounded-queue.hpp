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

#include <new>
#include <atomic>
#include <bit>
#include <array>

#include <abstract-task/abstract-task.hpp>

template <std::size_t Size>
class LFQueue
{
    static_assert(Size > 2, "Size must be > 2");
    static_assert(std::has_single_bit(Size), "Size must be power of two");

    [[nodiscard("Warning: more than 1 MB is allocated on the stack!")]] bool stack_allocation_warning() { return true; }

public:
    using abstract_task_t = abstract_task::Task<std::int32_t()>; // default return type -> 32-bit integer

public:
    LFQueue() : 
        mBufferMask{ Size - 1 }
    {
        if constexpr (sizeof(Node) * Size > sizeof(std::byte) * 1'024 * 1'024)
        {
            stack_allocation_warning();
        }

        mHead.store(0, std::memory_order_relaxed);
        mTail.store(0, std::memory_order_relaxed);

        for (std::size_t i{}; i < Size; ++i)
        {
            mBuffer[i].Sequence.store(i, std::memory_order_relaxed);
        }
    }

    ~LFQueue() noexcept = default;

    [[nodiscard]] bool TryPush(abstract_task_t& task)
    {
        auto old_tail_position{ mTail.load(std::memory_order_relaxed) };

        for (;;)
        {
            Node* node{ &mBuffer[old_tail_position & mBufferMask] };
            auto old_sequence{ node->Sequence.load(std::memory_order_acquire) };

            if (old_tail_position != old_sequence)
            {
                return false;
            }

            if (mTail.compare_exchange_weak(old_tail_position, old_tail_position + 1))
            {
                node->Task = std::move(task);
                node->Sequence.store(old_sequence + 1, std::memory_order_release);

                break;
            }
        }

        return true;
    }

    [[nodiscard]] bool TryPop(abstract_task_t& task)
    {
        auto old_head_position{ mHead.load(std::memory_order_relaxed) };
    
        for (;;)
        {
            Node* node{ &mBuffer[old_head_position & mBufferMask] };
            auto old_sequence{ node->Sequence.load(std::memory_order_acquire) };

            if (old_head_position + 1 != old_sequence) // old_head_position + 1 -> is there something in this node ?
            {
                return false;
            }
            if (IsEmpty())
            {
                return false;
            }

            if (mHead.compare_exchange_weak(old_head_position, old_head_position + 1))
            {
                task = std::move(node->Task);
                node->Sequence.store(old_head_position + Size, std::memory_order_release);

                break;
            }
        }

        return true;
    }

    [[nodiscard]] inline bool IsEmpty() const noexcept
    {
        return mHead.load(std::memory_order_acquire) == mTail.load(std::memory_order_acquire);
    }

private:
    struct alignas(std::hardware_destructive_interference_size) Node
    {
        Node() = default;
        ~Node() noexcept = default;

        abstract_task_t Task; 
        std::atomic<std::size_t> Sequence;
    };

    const std::size_t mBufferMask;

    alignas(std::hardware_destructive_interference_size) std::atomic<std::size_t> mHead;
    alignas(std::hardware_destructive_interference_size) std::atomic<std::size_t> mTail;

    alignas(std::hardware_destructive_interference_size) std::array<Node, Size> mBuffer;
};