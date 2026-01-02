#include <iostream>
#include "include/AdaptiveRadixTree.h"

int main() {
  PriceLevel lvl1, lvl2, lvl3;
  ARTreeWrapper art;

  art.insert(10050, &lvl1);
  art.insert(10075, &lvl2);
  art.insert(10025, &lvl3);

  art.printTreeStats();

  std::cout << "Max Tick: " << art.maxTick() << "\n";
  std::cout << "Min Tick: " << art.minTick() << "\n";

  void* found = art.search(10075);
  std::cout << "Search 10075 found? " << (found != nullptr) << "\n";

  art.remove(10075);
  art.printTreeStats();

  return 0;
}
