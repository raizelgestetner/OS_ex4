//
// Created by raize on 13/06/2023.
//

#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include "MemoryConstants.h"
#include <iostream>
//#include "YaaraTest/YaaraConstants.h"

///////function declarations/////////
void remove_hierarchy(uint64_t frame_index, int frameType[]);
void initialize_frame(uint64_t frame_idx);
//////////////////////////////////////

//long int frames_array[NUM_FRAMES] ; //contains PageIndex in each cell
//int frameType[NUM_FRAMES] = {0}; // 0 = initialized (not table and not data), 1 = table, 2 = data
int D_OFFSET = OFFSET_WIDTH;

int BITS_OF_PT_ADDR = OFFSET_WIDTH;
int SPAIR_BIT_OF_PT_ADDR = (VIRTUAL_ADDRESS_WIDTH - D_OFFSET) % OFFSET_WIDTH;
long int frames_array_global[NUM_FRAMES] ; //contains PageIndex in each cell
int frameType_global[NUM_FRAMES] = {0}; // 0 = initialized (not table and not data), 1 = table, 2 = data



void VMinitialize(){ // If no PM exist it will creat it in PhysicalMemory
  for (int i = 0; i < NUM_FRAMES; ++i)
    {
      frames_array_global[i] = -1;
      frameType_global[i] = 0;
    }

  initialize_frame (0);
  frameType_global[0] = 1;
}

uint64_t readBits(uint64_t number, uint64_t start, uint64_t end) {
  uint64_t mask = (1ULL << (end - start)) - 1;  // Create a mask of 1s with the desired width
  number >>= VIRTUAL_ADDRESS_WIDTH - end;  // Shift the number to align the desired bits to the right
  number &= mask;  // Apply the mask to extract the desired bits

  return number;
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

void print_frameType_array (int frameType[]){
  std::cout << "frameType"  << std::endl;

  for(int i = 0 ; i < NUM_FRAMES ; i++ ){
      std::cout << frameType[i] << " " ;
  }
  std::cout <<std::endl;
  std::cout << "frameType global"  << std::endl;

  for(int i = 0 ; i < NUM_FRAMES ; i++ ){
      std::cout << frameType_global[i] << " " ;

    }
  std::cout <<std::endl<<std::endl;
}
void print_frames_array ( long int frames_array[]){
  std::cout << "frames_array"  << std::endl;
  for(int i = 0 ; i < NUM_FRAMES ; i++ ){
      std::cout << frames_array[i] << " " ;

    }
    std::cout<<std::endl;
  std::cout << "frames_array global"  << std::endl;
  for(int i = 0 ; i < NUM_FRAMES ; i++ ){
      std::cout << frames_array_global[i] << " " ;
    }
  std::cout<<std::endl<<std::endl;
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

      uint64_t page_swapped_in = readBits(virtualAddress, 0, VIRTUAL_ADDRESS_WIDTH - D_OFFSET);

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
          frames_array_global[frame_index] = -1;
          print_frames_array (frames_array);
        }
      else if(frameType[frame_index] == 1){
          frameType[frame_index] = 0;
          frameType_global[frame_index] = 0;
          print_frameType_array (frameType);
        }
    }


  PMwrite(PAGE_SIZE * current_addr + offset , frame_index);

  if (data) {
      uint64_t pageAddress = readBits(virtualAddress, 0, VIRTUAL_ADDRESS_WIDTH - D_OFFSET);

      // load data page to frame_index
      PMrestore(frame_index, pageAddress);
      frameType[frame_index] = 2;
      frameType_global[frame_index] = 2;
      frames_array[frame_index] = pageAddress;
      frames_array_global[frame_index] = pageAddress;
      print_frameType_array (frameType);
      print_frames_array (frames_array);
    }
  else {
      frameType[frame_index] = 1;
      frameType_global[frame_index] = 1;
      print_frameType_array (frameType);
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

// Function to concatenate two bits into a uint64_t variable
uint64_t concatenateBits(uint64_t a, uint64_t b) {
  uint64_t result = (a << 1) | b;
  return result;
}

void recursive_travel(uint64_t pageIndex, uint64_t virtualAddress, int depth, int frameType[], long int frames_array[]) {

  std::cout << "IN RECUTSI"
//  word_t index_of_data;
//  uint64_t offset = readBits(virtualAddress, bits_length_start, bits_length_end);
//  PMread(pageIndex * PAGE_SIZE + offset ,&index_of_data);
//  frames_array[index_of_data] = pageIndex;

  // we already been here...
  if (frameType[pageIndex] != 0)
    return;

  if (depth == TABLES_DEPTH) {
      frameType[pageIndex] = 2;
      frames_array[pageIndex] = virtualAddress;
      return;
  }
  frameType[pageIndex] = 1;
  for (int j = 0; j < PAGE_SIZE; j++)
    {
      word_t tmp;
      PMread(pageIndex * PAGE_SIZE + j ,&tmp);
      if (tmp != 0)
        recursive_travel(tmp, concatenateBits(virtualAddress, j), ++depth, frameType, frames_array);
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
        recursive_travel(tmp, concatenateBits(0, j), 1, frameType, frames_array);
    }
}

int VMwrite(uint64_t virtualAddress, word_t value){

  long int frames_array[NUM_FRAMES] ; //contains PageIndex in each cell
  int frameType[NUM_FRAMES] = {0}; // 0 = initialized (not table and not data), 1 = table, 2 = data

  build_database (frameType, frames_array, virtualAddress);

  std::cout << "finished build arrays VMwrite" << std::endl;
  print_frameType_array (frameType);
  print_frames_array (frames_array);
  if (virtualAddress >= VIRTUAL_MEMORY_SIZE) {
      return 0;
    }

  int addr = 0;
  int tmp_addr = 0;

  int bits_length_start = 0;
  int bits_length_end = BITS_OF_PT_ADDR - SPAIR_BIT_OF_PT_ADDR;

  bool isData = false;
  for(int i = 0; i < TABLES_DEPTH; i++)
    {
      isData = i==TABLES_DEPTH-1;

      uint64_t offset = readBits(virtualAddress, bits_length_start, bits_length_end);
      bits_length_start = bits_length_end;
      bits_length_end += BITS_OF_PT_ADDR;
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

  uint64_t d = readBits (virtualAddress, VIRTUAL_ADDRESS_WIDTH - D_OFFSET, VIRTUAL_ADDRESS_WIDTH);
  PMwrite(addr * PAGE_SIZE + d ,value);
  return 1;
}

int VMread(uint64_t virtualAddress, word_t* value){

  if (virtualAddress >= VIRTUAL_MEMORY_SIZE) {
      return 0;
    }

  long int frames_array[NUM_FRAMES] = {-1} ; //contains PageIndex in each cell
  int frameType[NUM_FRAMES] = {0}; // 0 = initialized (not table and not data), 1 = table, 2 = data

  build_database (frameType, frames_array, virtualAddress);

  int addr = 0;
  int tmp_addr = 0;

  int bits_length_start = 0;
  int bits_length_end = BITS_OF_PT_ADDR - SPAIR_BIT_OF_PT_ADDR;
  bool isData = false;
  for(int i = 0; i < TABLES_DEPTH; i++)
    {
      isData = i==TABLES_DEPTH-1;
      uint64_t offset = readBits(virtualAddress, bits_length_start, bits_length_end);
      bits_length_start = bits_length_end;
      bits_length_end += BITS_OF_PT_ADDR;

      PMread(addr * PAGE_SIZE + offset ,&tmp_addr);
      if (!tmp_addr) {
          // load page to ram
          addr = handlePageLoad(addr, offset, isData, virtualAddress, frameType, frames_array);
        }
      else {
          addr = tmp_addr;
        }

    }

  uint64_t d = readBits (virtualAddress, VIRTUAL_ADDRESS_WIDTH - D_OFFSET, VIRTUAL_ADDRESS_WIDTH);
  PMread(addr * PAGE_SIZE + d,value);
  return 1;
}

