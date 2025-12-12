#pragma once
#include "PriceLevel.h"
#include <cstdint>
#include <cstddef>
#include <iostream>

extern "C"
{
#include <art.h>
}

class AdaptiveRadixTree
{
private:
    art_tree tree;

    static void makeKey(uint64_t price, unsigned char key[8]) noexcept
    {
        for (int i = 0; i < 8; i++)
        {
            key[i] = (price >> ((7 - i) * 8)) & 0xFF;
        }
    }

public:
    AdaptiveRadixTree() noexcept
    {
        art_tree_init(&tree);
    }

    ~AdaptiveRadixTree() noexcept
    {
        art_tree_destroy(&tree);
    }

    AdaptiveRadixTree(const AdaptiveRadixTree &) = delete;
    AdaptiveRadixTree &operator=(const AdaptiveRadixTree &) = delete;

    AdaptiveRadixTree(AdaptiveRadixTree &&) = delete;
    AdaptiveRadixTree &operator=(AdaptiveRadixTree &&) = delete;

    inline void insert(uint64_t price, PriceLevel *level)
    {
        unsigned char key[8];
        makeKey(price, key);
        void *old = art_insert(&tree, key, 8, level);
        if (old != nullptr)
        {
            throw std::logic_error("ART insert failed: duplicate price level");
        }
    }

    inline PriceLevel *find(uint64_t price) const noexcept
    {
        unsigned char key[8];
        makeKey(price, key);
        return static_cast<PriceLevel *>(art_search(&tree, key, 8));
    }

    inline PriceLevel *erase(uint64_t price)
    {
        unsigned char key[8];
        makeKey(price, key);
        void *removed = art_delete(&tree, key, 8);
        if (removed == nullptr)
        {
            throw std::logic_error("ART erase failed: key not found");
        }
        return static_cast<PriceLevel *>(removed);
    }

    inline uint64_t maxPrice() const noexcept
    {
        if (tree.size == 0)
            return 0;
        const art_leaf *leaf = art_maximum(const_cast<art_tree *>(&tree));
        uint64_t price = 0;
        for (int i = 0; i < 8; i++)
        {
            price = (price << 8) | leaf->key[i];
        }
        return price;
    }

    inline uint64_t minPrice() const noexcept
    {
        if (tree.size == 0)
            return 0;
        const art_leaf *leaf = art_minimum(const_cast<art_tree *>(&tree));
        uint64_t price = 0;
        for (int i = 0; i < 8; i++)
        {
            price = (price << 8) | leaf->key[i];
        }
        return price;
    }

    inline bool empty() const noexcept
    {
        return tree.size == 0;
    }

    inline size_t size() const noexcept
    {
        return tree.size;
    }

    art_tree *getTree() noexcept { return &tree; }

    void printTreeStats() const noexcept
    {
        std::cout << "[ART] size=" << tree.size << " minTick=" << minPrice()
                  << " maxTick=" << maxPrice() << "\n";
    }
};
