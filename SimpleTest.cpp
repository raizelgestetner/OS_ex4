#include "VirtualMemory.h"

#include <cstdio>
#include <cassert>

int main(int argc, char **argv) {

  // I try to test less iteration to debug. until i = 9 all good. if the loop is until 11
  // from some reason reading from 8 it curropted... by the way, 9 and 10 is ok also 8 not.
  // probably some memory violation... need to double check we dont write to wired palaces.
  // also - are we initilaize new frames when we find?! (on max_frame_id) i dont think so..

    VMinitialize();
    for (uint64_t i = 0; i < (2 * NUM_FRAMES); ++i) {
//    for (uint64_t i = 0; i < 11; ++i) {
        printf("writing to %llu\n", (long long int) i);
//        printf("virtual addres %llu\n", (long long int) 5 * i * PAGE_SIZE);
        VMwrite(5 * i * PAGE_SIZE, i);
    }

    for (uint64_t i = 0; i < (2 * NUM_FRAMES); ++i) {
//    for (uint64_t i = 0; i < 11; ++i) {
        word_t value;

//        printf("virtual addres %llu\n", (long long int) 5 * i * PAGE_SIZE);
        VMread(5 * i * PAGE_SIZE, &value);
        printf("reading from %llu %d\n", (long long int) i, value);
        assert(uint64_t(value) == i);
    }
    printf("success\n");

    return 0;
}
