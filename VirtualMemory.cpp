//
// Created by raize on 13/06/2023.
//

#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include "MemoryConstants.h"

///////function declarations/////////
void remove_hierarchy(uint64_t frame_index, int frameType[]);
void initialize_frame(uint64_t frame_idx);
//////////////////////////////////////

void VMinitialize(){ // If no PM exist it will creat it in PhysicalMemory
  initialize_frame (0);
}

uint64_t readBits(uint64_t number, uint64_t start, uint64_t end) {
  uint64_t mask = (1ULL << (end - start)) - 1;  // Create a mask of 1s with the desired width
  number >>= VIRTUAL_ADDRESS_WIDTH - end;  // Shift the number to align the desired bits to the right
  number &= mask;  // Apply the mask to extract the desired bits

  return number;
}

uint64_t concatenateBits(uint64_t a, uint64_t b, int bits_length) {
  const int a_bits_length = VIRTUAL_ADDRESS_WIDTH - bits_length;

  // Truncate the input numbers to the specified number of bits
  a &= ((1ULL << a_bits_length) - 1);
  b &= ((1ULL << bits_length) - 1);

  // Shift the bits of 'a' to the left by 'bits_length' positions
  a <<= bits_length;

  // Combine the truncated 'a' and 'b' using bitwise OR
  return a | b;
}

void recursive_travel(uint64_t pageIndex, uint64_t virtualAddress, int depth, int frameType[], long int frames_array[]) {

  if (TABLES_DEPTH == 0)
    return;
  // we already been here...
  if (frameType[pageIndex] != 0)
    return;

  if (depth == TABLES_DEPTH) {
      frameType[pageIndex] = 2;
      frames_array[pageIndex] = virtualAddress;
      return;
    }
  frameType[pageIndex] = 1;
  depth += 1;
  for (int j = 0; j < PAGE_SIZE; j++)
    {
      word_t tmp;
      PMread(pageIndex * PAGE_SIZE + j ,&tmp);
      if (tmp != 0)
        recursive_travel(tmp, concatenateBits(virtualAddress, j, OFFSET_WIDTH), depth, frameType, frames_array);
    }
}

void build_database(int frameType[], long int frames_array[], uint64_t virtualAddress) {

  for (int i = 0; i < NUM_FRAMES; ++i)
    {
      frames_array[i] = -1;
      frameType[i] = 0;
    }

  frameType[0] = 1;
  frames_array[0] = -1;

  for (int j = 0; j < PAGE_SIZE; j++)
    {
      word_t tmp;
      PMread(j ,&tmp);
      if (tmp != 0)
        recursive_travel(tmp, concatenateBits(0, j, OFFSET_WIDTH), 1, frameType, frames_array);
    }
}

void findEmptyTable(uint64_t &frame_index, uint64_t &max_frame_id, int current_addr, int frameType[]) {

  // find free space to load from swap
  frame_index = 0;
  max_frame_id = current_addr;
  word_t tmp;
  for (int i = 0; i < NUM_FRAMES; i++)
    {
      bool all_rows_empty = true;
      if(frameType[i] == 1){
          for (int j = 0; j < PAGE_SIZE; j++)
            {
              PMread(i * PAGE_SIZE + j ,&tmp);
              if((int)max_frame_id < tmp)
                max_frame_id = tmp;
              if (tmp && all_rows_empty)
                all_rows_empty = false;
            }
          if (all_rows_empty && i != current_addr)
            {
              frame_index = i;
              break;
            }
        }
    }
}

uint64_t abs_minus_64(uint64_t a,uint64_t b){
  if(a > b )
    return a-b;
  return b-a;
}



uint64_t handlePageLoad(int current_addr, uint64_t offset, int data, uint64_t virtualAddress,
                        int frameType[], long int frames_array[]) {
  // We want to found new place to write the tables
  // Try to find empty table
  uint64_t frame_index;
  uint64_t max_frame_id;
  findEmptyTable(frame_index, max_frame_id, current_addr, frameType);

  if (frame_index){
      remove_hierarchy (frame_index, frameType);
    }
  else if (max_frame_id+1 < NUM_FRAMES) {
      max_frame_id ++;
      frame_index = max_frame_id;


    }
  else {
      uint64_t min = 0;
      uint64_t last_min = 0;

      uint64_t page_swapped_in = readBits(virtualAddress, 0, VIRTUAL_ADDRESS_WIDTH - OFFSET_WIDTH);

      for (int tmp_frame_idx = 1; tmp_frame_idx < NUM_FRAMES; ++tmp_frame_idx)
        {
          if(frames_array[tmp_frame_idx] != -1) // there is a page in that frame
            {
              uint64_t page_idx = frames_array[tmp_frame_idx];
              min = NUM_PAGES - abs_minus_64 (page_swapped_in ,page_idx);
              uint64_t tmp = abs_minus_64 (page_swapped_in, page_idx);
              if (tmp < min)
                {
                  min = tmp;
                }
              if(min > last_min)
                {
                  last_min = min;
                  frame_index = tmp_frame_idx;
                }
            }
        }


      // swap page p
      if(frameType[frame_index] == 2){ // If frame we want to evict is data, we'll save it into the disk.(It should be!)
          uint64_t evictedPageIndex = frames_array[frame_index];
          PMevict(frame_index, evictedPageIndex);
          remove_hierarchy(frame_index, frameType);
          frames_array[frame_index] = -1;
        }
      else if(frameType[frame_index] == 1){
          frameType[frame_index] = 0;
        }
    }

  PMwrite(PAGE_SIZE * current_addr + offset , frame_index);

  if (data) {
      uint64_t pageAddress = readBits(virtualAddress, 0, VIRTUAL_ADDRESS_WIDTH - OFFSET_WIDTH);

      // load data page to frame_index
      PMrestore(frame_index, pageAddress);
      frameType[frame_index] = 2;
      frames_array[frame_index] = pageAddress;
    }
  else {
      frameType[frame_index] = 1;
    }


  if (frameType[frame_index] == 1) {
      initialize_frame(frame_index);
    }

  return frame_index;
}

void initialize_frame(uint64_t frame_idx){
  for (int i = 0; i < PAGE_SIZE; ++i)
    {
      PMwrite(frame_idx * PAGE_SIZE + i, 0); //  clear every address in frame 0
    }
}

void remove_hierarchy(uint64_t frame_index, int frameType[]){
  word_t value;
  for (uint64_t i = 0; i < NUM_FRAMES; ++i)
    {
      if(frameType[i] == 1){ // if the frame is a table
          for (uint64_t j = 0; j < PAGE_SIZE; ++j)
            {
              PMread (i * PAGE_SIZE + j, &value);
              if(value == (int)frame_index){
                  PMwrite (i * PAGE_SIZE + j, 0);
                }
            }

        }
    }
}

int VMwrite(uint64_t virtualAddress, word_t value){
  int SPAIR_BIT_OF_PT_ADDR = (VIRTUAL_ADDRESS_WIDTH - OFFSET_WIDTH) % OFFSET_WIDTH;
  long int frames_array[NUM_FRAMES] ; //contains PageIndex in each cell
  int frameType[NUM_FRAMES] = {0}; // 0 = initialized (not table and not data), 1 = table, 2 = data

  build_database (frameType, frames_array, virtualAddress);


  if (virtualAddress >= VIRTUAL_MEMORY_SIZE) {
      return 0;
    }

  int addr = 0;
  int tmp_addr = 0;

  int bits_length_start = 0;
  int bits_length_end = OFFSET_WIDTH - SPAIR_BIT_OF_PT_ADDR;

  bool isData = false;
  for(int i = 0; i < TABLES_DEPTH; i++)
    {
      isData = i==TABLES_DEPTH-1;

      uint64_t offset = readBits(virtualAddress, bits_length_start, bits_length_end);
      bits_length_start = bits_length_end;
      bits_length_end += OFFSET_WIDTH;
      PMread(addr * PAGE_SIZE + offset ,&tmp_addr);
      if (!tmp_addr) {
          // load page to ram
          addr = handlePageLoad(addr, offset, isData, virtualAddress, frameType, frames_array);
        }
      else
        {
          addr = tmp_addr;
        }

    }

  uint64_t d = readBits (virtualAddress, VIRTUAL_ADDRESS_WIDTH - OFFSET_WIDTH, VIRTUAL_ADDRESS_WIDTH);
  PMwrite(addr * PAGE_SIZE + d ,value);
  return 1;
}

int VMread(uint64_t virtualAddress, word_t* value){
  int SPAIR_BIT_OF_PT_ADDR = (VIRTUAL_ADDRESS_WIDTH - OFFSET_WIDTH) % OFFSET_WIDTH;
  if (virtualAddress >= VIRTUAL_MEMORY_SIZE) {
      return 0;
    }

  long int frames_array[NUM_FRAMES] = {-1} ; //contains PageIndex in each cell
  int frameType[NUM_FRAMES] = {0}; // 0 = initialized (not table and not data), 1 = table, 2 = data

  build_database (frameType, frames_array, virtualAddress);

  int addr = 0;
  int tmp_addr = 0;

  int bits_length_start = 0;
  int bits_length_end = OFFSET_WIDTH - SPAIR_BIT_OF_PT_ADDR;
  bool isData = false;
  for(int i = 0; i < TABLES_DEPTH; i++)
    {
      isData = i==TABLES_DEPTH-1;
      uint64_t offset = readBits(virtualAddress, bits_length_start, bits_length_end);
      bits_length_start = bits_length_end;
      bits_length_end += OFFSET_WIDTH;

      PMread(addr * PAGE_SIZE + offset ,&tmp_addr);
      if (!tmp_addr) {
          // load page to ram
          addr = handlePageLoad(addr, offset, isData, virtualAddress, frameType, frames_array);
        }
      else {
          addr = tmp_addr;
        }

    }

  uint64_t d = readBits (virtualAddress, VIRTUAL_ADDRESS_WIDTH - OFFSET_WIDTH, VIRTUAL_ADDRESS_WIDTH);
  PMread(addr * PAGE_SIZE + d,value);
  return 1;
}
