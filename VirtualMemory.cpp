//
// Created by raize on 13/06/2023.
//

#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include "MemoryConstants.h"

char VM[NUM_PAGES][PAGE_SIZE];

void VMinitialize(){ // If no PM exist it will creat it in PhysicalMemory
  VM = {0};
  for (int i = 0; i < PAGE_SIZE; ++i)
    {
      PMwrite(0 + i, 0) //  clear every address in frame 0
    }
}

int VMwrite(uint64_t virtualAddress, word_t value){

}


