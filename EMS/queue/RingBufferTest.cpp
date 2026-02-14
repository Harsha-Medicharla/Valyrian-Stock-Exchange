#include "RingBuffer.h"
#include <iostream>

int main() {
    RingBuffer<int, 8> rb;

    for (int i = 0; i < 8; ++i) {
        if (!rb.push(i)) {
            std::cout << "Push failed at " << i << "\n";
        }
    }

    int value;
    while (rb.pop(value)) {
        std::cout << value << " ";
    }

    return 0;
}
