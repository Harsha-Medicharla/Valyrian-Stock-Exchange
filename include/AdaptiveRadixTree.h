#pragma once
#include "DataStructures.h"
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>

extern "C" {
#include "../libs/art/src/art.h"
}

class ARTreeWrapper {
private:
  art_tree tree;

  static void makeKey(uint64_t ticks, unsigned char key[8]) {
    for (int i = 0; i < 8; i++) {
      key[i] = (ticks >> ((7 - i) * 8)) & 0xFF;
    }
  }

public:
  ARTreeWrapper() { art_tree_init(&tree); }

  void insert(uint64_t priceTicks, PriceLevel *value) {
    unsigned char key[8];
    makeKey(priceTicks, key);
    art_insert(&tree, key, 8, value);
  }

  uint64_t maxTick() const {
    if (tree.size == 0)
      return 0;
    art_leaf *leaf = art_maximum(const_cast<art_tree *>(&tree));

    uint64_t ticks = 0;
    for (int i = 0; i < 8; i++) {
      ticks = (ticks << 8) | leaf->key[i];
    }
    return ticks;
  }

  uint64_t minTick() const {
    if (tree.size == 0)
      return 0;
    art_leaf *leaf = art_minimum(const_cast<art_tree *>(&tree));

    uint64_t ticks = 0;
    for (int i = 0; i < 8; i++) {
      ticks = (ticks << 8) | leaf->key[i];
    }
    return ticks;
  }

  // Lookup stored value (raw pointer)
  void *search(uint64_t priceTicks) const {
    unsigned char key[8];
    makeKey(priceTicks, key);
    return art_search(&tree, key, 8);
  }

  // Remove a price level
  void remove(uint64_t priceTicks) {
    unsigned char key[8];
    makeKey(priceTicks, key);
    void *deleted = art_delete(&tree, key, 8);
    if (deleted == 0) {
      throw std::runtime_error("ART remove failed: key not found");
    }
  }

  bool isEmpty() const { return tree.size == 0; }
  size_t size() const { return tree.size; }

  art_tree *getTree() { return &tree; }

  // For testing output convenience
  void printTreeStats() const {
    std::cout << "[ART] size=" << tree.size << " minTick=" << minTick()
              << " maxTick=" << maxTick() << "\n";
  }
};
