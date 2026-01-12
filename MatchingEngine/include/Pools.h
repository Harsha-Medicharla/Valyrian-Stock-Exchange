#pragma once
#include <cstddef>
#include "PriceLevel.h"
#include <stdexcept>

static constexpr size_t MAX_ORDERS = 1000000;
static constexpr size_t MIN_REQ_PRICELEVELS = 1000000;
class OrderPool
{
private:
    Order pool[MAX_ORDERS];
    Order *free_list[MAX_ORDERS];
    size_t top;

public:
    OrderPool()
    {
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
    PriceLevel pool[MIN_REQ_PRICELEVELS];
    PriceLevel *free_list[MIN_REQ_PRICELEVELS];
    size_t top;

public:
    PriceLevelPool()
    {
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
