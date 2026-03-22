#pragma once
#include <cstddef>
#include "PriceLevel.h"
#include <stdexcept>
#include <memory> 

static constexpr size_t MAX_ORDERS = 1000000;
static constexpr size_t MIN_REQ_PRICELEVELS = 1000000;

class OrderPool
{
private:
    std::unique_ptr<Order[]> pool;
    std::unique_ptr<Order*[]> free_list;
    size_t top;

public:
    OrderPool()
    {
        pool = std::make_unique<Order[]>(MAX_ORDERS);
        free_list = std::make_unique<Order*[]>(MAX_ORDERS);
        
        top = MAX_ORDERS;
        for (size_t i = 0; i < MAX_ORDERS; i++)
        {
            free_list[i] = &pool[i];
        }
    }

    Order *allocate()
    {
        if (top == 0)
        {
            throw std::runtime_error("OrderPool exhausted");
        }
        return free_list[--top];
    }

    void deallocate(Order *order)
    {
        if (order == nullptr)
        {
            throw std::logic_error("Attempt to deallocate null Order");
        }
        if (top >= MAX_ORDERS)
        {
            throw std::logic_error("OrderPool overflow (double free?)");
        }
        order->reset();
        free_list[top++] = order;
    }
};

class PriceLevelPool
{
private:
    std::unique_ptr<PriceLevel[]> pool;
    std::unique_ptr<PriceLevel*[]> free_list;
    size_t top;

public:
    PriceLevelPool()
    {
        pool = std::make_unique<PriceLevel[]>(MIN_REQ_PRICELEVELS);
        free_list = std::make_unique<PriceLevel*[]>(MIN_REQ_PRICELEVELS);
        
        top = MIN_REQ_PRICELEVELS;
        for (size_t i = 0; i < MIN_REQ_PRICELEVELS; i++)
        {
            free_list[i] = &pool[i];
        }
    }

    PriceLevel *allocate()
    {
        if (top == 0)
        {
            throw std::runtime_error("PriceLevelPool exhausted");
        }
        return free_list[--top];
    }

    void deallocate(PriceLevel *level)
    {
        if (level == nullptr)
        {
            throw std::logic_error("Attempt to deallocate null PriceLevel");
        }
        if (top >= MIN_REQ_PRICELEVELS)
        {
            throw std::logic_error("PriceLevelPool overflow (double free?)");
        }
        level->reset();
        free_list[top++] = level;
    }
};