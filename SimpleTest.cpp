#include "VirtualMemory.h"

#include <cstdio>
#include <cassert>

int main(int argc, char **argv) {


    VMinitialize();
//    for (uint64_t i = 0; i < (2 * NUM_FRAMES); ++i) {
    for (uint64_t i = 0; i < 11; ++i) {
        printf("writing to %llu\n", (long long int) i);
        printf("virtual addres %llu\n", (long long int) 5 * i * PAGE_SIZE);
        VMwrite(5 * i * PAGE_SIZE, i);
    }

//    for (uint64_t i = 0; i < (2 * NUM_FRAMES); ++i) {
    for (uint64_t i = 0; i < 11; ++i) {
        word_t value;
        printf("reading from %llu %d\n", (long long int) i, value);
        printf("virtual addres %llu\n", (long long int) 5 * i * PAGE_SIZE);
        VMread(5 * i * PAGE_SIZE, &value);
        assert(uint64_t(value) == i);
    }
    printf("success\n");

    return 0;
}
