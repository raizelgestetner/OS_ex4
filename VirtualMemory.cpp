//
// Created by raize on 13/06/2023.
//

#include <iostream>
#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include "MemoryConstants.h"
#include "cmath"

uint64_t frames_array[RAM_SIZE]; //RAM MEM
//TODO: check raizel is correct?
bool frameType[NUM_FRAMES] = {false};
uint64_t pagesIndexes[NUM_PAGES];
int D_OFFSET = log2(PAGE_SIZE); // bits of data page offset
int DEPTH_OF_PT_TREE = CEIL((log2(VIRTUAL_MEMORY_SIZE) - log2(PAGE_SIZE)) / log2(PAGE_SIZE));

// TODO: check! BITS_OF_PT_ADDR can be double?!?!
int BITS_OF_PT_ADDR = (VIRTUAL_ADDRESS_WIDTH - D_OFFSET) / DEPTH_OF_PT_TREE;

void VMinitialize(){ // If no PM exist it will creat it in PhysicalMemory

  for (int i = 0; i < PAGE_SIZE; ++i)
    {
      PMwrite(0 + i, 0); //  clear every address in frame 0
    }
  frameType[0] = true;
}

uint64_t readBits(uint64_t number, int start, int end) {
  unsigned long long mask = 0;
  int rangeLength = end - start + 1;

  for (int i = 0; i < rangeLength; i++) {
      mask |= (1ULL << i);
    }

  number >>= start;
  return number & mask;
}

//TODO: need reference or actual variable?!
void findEmptyTable(uint64_t &frame_index, uint64_t &max_frame_id) {

  // find free space to load from swap
  // TODO: run only on tables
  frame_index = 0;
  max_frame_id = 0;
  bool all_rows_empty = true;
  word_t tmp;
  for (int i = 0; i < NUM_FRAMES; i++)
    {
      if(frameType[i]){
        for (int j = 0; j < PAGE_SIZE; j++)
          {
            //tmp = frames_array[i * PAGE_SIZE + j * PHYSICAL_ADDRESS_WIDTH];
            PMread(i * PAGE_SIZE + j * PHYSICAL_ADDRESS_WIDTH,&tmp);
            if(max_frame_id < tmp)
              max_frame_id = tmp;
            if (tmp)
              all_rows_empty = false;
          }
        if (all_rows_empty)
          {
            frame_index = i;
            break;
          }
      }
    }
}

int handlePageLoad(int current_addr, uint64_t offset, int data, uint64_t virtualAddress) {
  // We want to found new place to write the tables
  // Try to find empty table
    uint64_t frame_index;
    uint64_t max_frame_id;
    findEmptyTable(frame_index, max_frame_id);

    if (frame_index)
      {
        // write
        PMwrite(PAGE_SIZE * current_addr + offset, frame_index);
      }
    else if (max_frame_id+1 < NUM_FRAMES) {
        frame_index = max_frame_id+1;
        PMwrite(PAGE_SIZE * current_addr + offset, frame_index);
    }
    else {
      int min = 0;
      int page_swaped_in = 0;

      // find p page to swaped
      for (int p =0; p < NUM_PAGES; p++) {
        min = NUM_PAGES - abs (page_swaped_in - p);
        int tmp = abs (page_swaped_in - p);
        if (tmp < min)
          min = tmp;
      }

//       swap page p
        uint64_t pageId = readBits(virtualAddress, 0, VIRTUAL_ADDRESS_WIDTH - D_OFFSET);
//        int pageId = 0;
//        for (int i = 0; i < NUM_PAGES; i++)
//          {
//            uint64_t pageAddress = readBits(virtualAddress, 0, VIRTUAL_ADDRESS_WIDTH - D_OFFSET);
//            if (pagesIndexes[i] == pageAddress)
//              {
//                pageId = i;
//                break;
//              }
//          }
      PMevict(min, pageId);
    }

  if (data) {
    // load data page to frame_index
    int pageId = -1;
    int emptyIndex = -1;
    for (int i = 0; i < NUM_PAGES; i++)
      {
        uint64_t pageAddress = readBits(virtualAddress, 0, VIRTUAL_ADDRESS_WIDTH - D_OFFSET);
        if (pagesIndexes[i] == pageAddress || !pagesIndexes[i])
          {
            pageId = i;
            pagesIndexes[i] = pageAddress;
            break;
          }
      }
      if (pageId == -1)
        {
          PMrestore(frame_index, pageId);
          frameType[frame_index] = false;
        }
  }
  else {
      frameType[frame_index] = true;
  }
  return frame_index;
}

int VMwrite(uint64_t virtualAddress, word_t value){

  int addr = 0;
  int tmp_addr = 0;
  for(int i = 0; i < DEPTH_OF_PT_TREE-1; i++)
    {
      //TODO: from some reason, when we write the second time (the second loop in the test)
      //TODO: we get offset = 16. its not make sense because the page size is 16! so we actually read from next page...
      // TODO: I try to fix but now its 0 from some reason.. its seams it shuld be 10 at this point.
      // TODO: 80 is 1010000, we want the first 4 digit 1010 with is 10...
      uint64_t offset = readBits(virtualAddress, i * BITS_OF_PT_ADDR, (i+1) * BITS_OF_PT_ADDR - 1);

      PMread(addr * PAGE_SIZE + offset,&tmp_addr);

      if (!tmp_addr) {
          // load page to ram
          addr = handlePageLoad(addr, offset, false, virtualAddress);
      }
      std::cout << "addr: " << addr << ", depth: " << i << std::endl;
    }

  uint64_t offset = readBits(virtualAddress, (DEPTH_OF_PT_TREE-1) * BITS_OF_PT_ADDR, (DEPTH_OF_PT_TREE) * BITS_OF_PT_ADDR);

  PMread(addr * PAGE_SIZE + offset,&tmp_addr);

  if (!tmp_addr) {
      // load page to ram
      addr = handlePageLoad(addr, offset, true, virtualAddress);
    }

  uint64_t d = readBits (virtualAddress, VIRTUAL_ADDRESS_WIDTH - D_OFFSET, VIRTUAL_ADDRESS_WIDTH);
  PMwrite(addr*PAGE_SIZE+d,value);

  return 1;
}

int VMread(uint64_t virtualAddress, word_t* value){

  int addr = 0;
  int tmp_addr = 0;
  for(int i = 0; i < DEPTH_OF_PT_TREE-1; i++)
    {
      uint64_t offset = readBits(virtualAddress, i * BITS_OF_PT_ADDR, (i+1) * BITS_OF_PT_ADDR);

      PMread(addr * PAGE_SIZE + offset,&tmp_addr);

      if (!tmp_addr) {
          // load page to ram
          addr = handlePageLoad(addr, offset, false, virtualAddress);
        }
    }

  uint64_t offset = readBits(virtualAddress, (DEPTH_OF_PT_TREE-1) * BITS_OF_PT_ADDR, (DEPTH_OF_PT_TREE) * BITS_OF_PT_ADDR);

  PMread(addr * PAGE_SIZE + offset,&tmp_addr);

  if (!tmp_addr) {
      // load page to ram
      addr = handlePageLoad(addr, offset, true, virtualAddress);
    }

  uint64_t d = readBits (virtualAddress, VIRTUAL_ADDRESS_WIDTH - D_OFFSET, VIRTUAL_ADDRESS_WIDTH);
  PMread(addr*PAGE_SIZE+d,value);

  return 1;
}


