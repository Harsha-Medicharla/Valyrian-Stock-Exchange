// common/Pool.h
#pragma once
#include <vector>
#include <atomic>
#include <cassert>
#include <optional>

// A simplified High-Performance pre-allocated Pool & Queue.
// For production HFT, consider replacing the push/pop logic with 
// moodycamel::ConcurrentQueue, but this serves the architectural interface.
template <typename T>
class Pool {
public:
    explicit Pool(size_t size) : capacity(size), head(0), tail(0) {
        memory_block.resize(size);
        free_list.resize(size);
        for (size_t i = 0; i < size; ++i) {
            free_list[i] = &memory_block[i];
        }
        free_index = size - 1;
        
        queue.resize(size, nullptr);
    }

    // --- MEMORY ALLOCATION ---
    // Called by the Producer (e.g., ME thread for Q4)
    T* allocate() {
        size_t current_free = free_index.fetch_sub(1, std::memory_order_acquire);
        if (current_free == static_cast<size_t>(-1)) {
            free_index.fetch_add(1, std::memory_order_relaxed);
            return nullptr; // Pool exhausted
        }
        return free_list[current_free];
    }

    // Called by the Consumer (Settlement thread) when done
    void deallocate(T* obj) {
        size_t next_free = free_index.fetch_add(1, std::memory_order_release) + 1;
        free_list[next_free] = obj;
    }

    // --- THREAD-SAFE QUEUE ---
    // Pushes to the queue
    bool push(T* item) {
        size_t current_tail = tail.load(std::memory_order_relaxed);
        size_t next_tail = (current_tail + 1) % capacity;
        
        if (next_tail == head.load(std::memory_order_acquire)) {
            return false; // Queue full
        }
        
        queue[current_tail] = item;
        tail.store(next_tail, std::memory_order_release);
        return true;
    }

    // Pops from the queue
    bool pop(T*& item) {
        size_t current_head = head.load(std::memory_order_relaxed);
        
        if (current_head == tail.load(std::memory_order_acquire)) {
            return false; // Queue empty
        }
        
        item = queue[current_head];
        head.store((current_head + 1) % capacity, std::memory_order_release);
        return true;
    }

private:
    size_t capacity;
    
    // Memory pooling
    std::vector<T> memory_block;
    std::vector<T*> free_list;
    std::atomic<size_t> free_index;

    // Ring buffer queue
    std::vector<T*> queue;
    std::atomic<size_t> head;
    std::atomic<size_t> tail;
};