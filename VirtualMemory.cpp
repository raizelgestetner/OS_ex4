//
// Created by raize on 13/06/2023.
//

#include <iostream>
#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include "MemoryConstants.h"
//#include "YaaraTest/YaaraConstants.h"
#include "cmath"

///////function declarations/////////
void remove_hierarchy(uint64_t frame_index);
void initialize_frame(uint64_t frame_idx);
//////////////////////////////////////

long int frames_array[NUM_FRAMES] ; //contains PageIndex in each cell
//TODO: check raizel is correct?
int frameType[NUM_FRAMES] = {0}; // 0 = initialized (not table and not data), 1 = table, 2 = data
//uint64_t pagesIndexes[NUM_PAGES];

//int D_OFFSET = log2(PAGE_SIZE); // bits of data page offset
int D_OFFSET = OFFSET_WIDTH;
int DEPTH_OF_PT_TREE = CEIL((log2(VIRTUAL_MEMORY_SIZE) - log2(PAGE_SIZE)) / log2(PAGE_SIZE));

// TODO: check! BITS_OF_PT_ADDR can be double?!?!
int BITS_OF_PT_ADDR = CEIL((double)(VIRTUAL_ADDRESS_WIDTH - D_OFFSET) / DEPTH_OF_PT_TREE);
int SPAIR_BIT_OF_PT_ADDR = (VIRTUAL_ADDRESS_WIDTH - D_OFFSET) % DEPTH_OF_PT_TREE;

void VMinitialize(){ // If no PM exist it will creat it in PhysicalMemory

//  std::cout << "ini!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
  for (int i = 0; i < NUM_FRAMES; ++i)
    {
      frames_array[i] = -1;
      frameType[i] = 0;
    }

  initialize_frame (0);
  frameType[0] = 1;
}

uint64_t readBits(uint64_t number, uint64_t start, uint64_t end) {
  uint64_t mask = (1ULL << (end - start)) - 1;  // Create a mask of 1s with the desired width
  number >>= VIRTUAL_ADDRESS_WIDTH - end;  // Shift the number to align the desired bits to the right
  number &= mask;  // Apply the mask to extract the desired bits

  return number;
}



//TODO: need reference or actual variable?!
void findEmptyTable(uint64_t &frame_index, uint64_t &max_frame_id, int current_addr) {

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
              if(max_frame_id < tmp)
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

uint64_t handlePageLoad(int current_addr, uint64_t offset, int data, uint64_t virtualAddress) {
  // We want to found new place to write the tables
  // Try to find empty table
  uint64_t frame_index;
  uint64_t max_frame_id;
  findEmptyTable(frame_index, max_frame_id, current_addr);

  if (frame_index){
//      std::cout << "1" << std::endl;
      remove_hierarchy (frame_index);
  }
  else if (max_frame_id+1 < NUM_FRAMES) {
//      std::cout << "2" << std::endl;
      max_frame_id ++;
      frame_index = max_frame_id;


//      initialize_frame(frame_index);
    }
  else {
//      std::cout << "3" << std::endl;
      uint64_t min = 0;
      uint64_t last_min = 0;

      // int page_swaped_in = 0;
      uint64_t page_swapped_in = readBits(virtualAddress, 0, VIRTUAL_ADDRESS_WIDTH - D_OFFSET);

      // find p page to swaped without frame 0
      for (int tmp_frame_idx = 1; tmp_frame_idx < NUM_FRAMES; ++tmp_frame_idx)
        {
//          std::cout << "frames_array[tmp_frame_idx] at: " << tmp_frame_idx << ", valuse: " << frames_array[tmp_frame_idx] << std::endl;
          if(frames_array[tmp_frame_idx] != -1) // there is a page in that frame
//          if(frameType[tmp_frame_idx] == 2) // there is a page in that frame
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
//                std::cout << "found page at: " << tmp_frame_idx << std::endl;
            }
//          else if(frameType[tmp_frame_idx] == 2){
//            if (!frame_index)
//              {
//                std::cout << "frameType[frame_index]" << frameType[tmp_frame_idx] << " of tmp_frame_idx: " << tmp_frame_idx << std::endl;
//                std::cout << "-1 tmp_frame_idx is: " << tmp_frame_idx << std::endl;
//                frame_index = tmp_frame_idx;
//                break;
//              }
//          }
        }


      // swap page p
      if(frameType[frame_index] == 2){ // If frame we want to evict is data, we'll save it into the disk.(It should be!)
          uint64_t evictedPageIndex = frames_array[frame_index];
          PMevict(frame_index, evictedPageIndex);
          remove_hierarchy(frame_index);
//          initialize_frame(frame_index);
          frames_array[frame_index] = -1;
//          std::cout << "frame evicated: " << frame_index << " index" << std::endl;

//          if (!data)
//            initialize_frame(frame_index);
      }
      else if(frameType[frame_index] == 1){
//          for (int i = 0; i < PAGE_SIZE; ++i)
//            {
//              PMwrite(frame_index + i, 0); //  clear every address in frame frame_index
//            }
//          initialize_frame(frame_index);
          frameType[frame_index] = 0;
      }
//      else {
//          initialize_frame(frame_index);
//      }
    }


  PMwrite(PAGE_SIZE * current_addr + offset , frame_index);

  if (data) {
      uint64_t pageAddress = readBits(virtualAddress, 0, VIRTUAL_ADDRESS_WIDTH - D_OFFSET);

      // load data page to frame_index
      PMrestore(frame_index, pageAddress);
      frameType[frame_index] = 2;
      frames_array[frame_index] = pageAddress;
//      std::cout << "save data to " << frame_index << " index, set addres: " << frames_array[frame_index] << std::endl;

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

void remove_hierarchy(uint64_t frame_index){
  word_t value;
  for (uint64_t i = 0; i < NUM_FRAMES; ++i)
    {
      if(frameType[i] == 1){ // if the frame is a table
          for (uint64_t j = 0; j < PAGE_SIZE; ++j)
            {
//              std::cout << "remove_hierarchy1" << std::endl <<std::flush;
              PMread (i * PAGE_SIZE + j, &value);
//              std::cout << "remove_hierarchy2" << std::endl <<std::flush;
              if(value == frame_index){
                  PMwrite (i * PAGE_SIZE + j, 0);
              }
            }

      }
    }
}

int VMwrite(uint64_t virtualAddress, word_t value){

  if (virtualAddress >= VIRTUAL_MEMORY_SIZE) {
      return 0;
  }

  int addr = 0;
  int tmp_addr = 0;

  int bits_length_start = 0;
  int bits_length_end = BITS_OF_PT_ADDR - SPAIR_BIT_OF_PT_ADDR;

  for(int i = 0; i < DEPTH_OF_PT_TREE-1; i++)
    {

//      uint64_t offset = readBits(virtualAddress, i * bits_length, (i+1) * bits_length);
      uint64_t offset = readBits(virtualAddress, bits_length_start, bits_length_end);
//      std::cout << "bits_length_start: " << bits_length_start << " bits_length_end: " << bits_length_end << ", offset: " << offset << std::endl;
      bits_length_start = bits_length_end;
      bits_length_end += BITS_OF_PT_ADDR;
      PMread(addr * PAGE_SIZE + offset ,&tmp_addr);

      if (!tmp_addr) {
          // load page to ram
          addr = handlePageLoad(addr, offset, false, virtualAddress);
        }
      else
        {
          addr = tmp_addr;
        }

//      std::cout << "in table " << i << ", offset: " << offset << ". we write" << " addr: " << addr << ", depth: " << i << std::endl;
    }

  uint64_t offset = readBits(virtualAddress, bits_length_start, bits_length_end);
//  std::cout << "bits_length_start: " << bits_length_start << " bits_length_end: " << bits_length_end << ", offset: " << offset << std::endl;
//  std::cout << "bits_length: " << BITS_OF_PT_ADDR << ", at i: " << (DEPTH_OF_PT_TREE-1) << ", offset: " << offset << std::endl;
//  uint64_t d = readBits (virtualAddress, VIRTUAL_ADDRESS_WIDTH - D_OFFSET, VIRTUAL_ADDRESS_WIDTH);

  PMread(addr * PAGE_SIZE + offset ,&tmp_addr);

  if (!tmp_addr) {
      // load page to ram
      addr = handlePageLoad(addr, offset, true, virtualAddress);
//      std::cout << "data find frame addr: " << addr << std::endl;
    }
  else
    {
      addr = tmp_addr;
    }
//  std::cout << "in table " << 3 << ", offset: " << offset << ". we write" << " data addr: " << addr << ", depth: " << 4 << std::endl;

  uint64_t d = readBits (virtualAddress, VIRTUAL_ADDRESS_WIDTH - D_OFFSET, VIRTUAL_ADDRESS_WIDTH);
  PMwrite(addr * PAGE_SIZE + d ,value);


//  std::cout << "data to: " << addr << ", offset: " << d << ", value: " << value << ", depth: " << 4 << std::endl;

  return 1;
}

int VMread(uint64_t virtualAddress, word_t* value){

  if (virtualAddress >= VIRTUAL_MEMORY_SIZE) {
      return 0;
    }

  int addr = 0;
  int tmp_addr = 0;

  int bits_length_start = 0;
  int bits_length_end = BITS_OF_PT_ADDR - SPAIR_BIT_OF_PT_ADDR;

  for(int i = 0; i < DEPTH_OF_PT_TREE-1; i++)
    {

//      std::cout << "BITS_OF_PT_ADDR" << BITS_OF_PT_ADDR << "SPAIR_BIT_OF_PT_ADDR" << SPAIR_BIT_OF_PT_ADDR << std::endl;
//      std::cout << "VIRTUAL_ADDRESS_WIDTH" << VIRTUAL_ADDRESS_WIDTH << "(VIRTUAL_ADDRESS_WIDTH - D_OFFSET)" << (VIRTUAL_ADDRESS_WIDTH - D_OFFSET) << "DEPTH_OF_PT_TREE" << DEPTH_OF_PT_TREE << std::endl;
//      std::cout << "CEIL((VIRTUAL_ADDRESS_WIDTH - D_OFFSET) / DEPTH_OF_PT_TREE) " << CEIL((double)(VIRTUAL_ADDRESS_WIDTH - D_OFFSET) / DEPTH_OF_PT_TREE) << std::endl;

//      uint64_t offset = readBits(virtualAddress, i * bits_length, (i+1) * bits_length);
      uint64_t offset = readBits(virtualAddress, bits_length_start, bits_length_end);
//      std::cout << "bits_length_start: " << bits_length_start << " bits_length_end: " << bits_length_end << ", offset: " << offset << std::endl;
      bits_length_start = bits_length_end;
      bits_length_end += BITS_OF_PT_ADDR;
//      std::cout << "bits_length: " << bits_length << ", at i: " << i << std::endl;

      PMread(addr * PAGE_SIZE + offset ,&tmp_addr);

      if (!tmp_addr) {
          // load page to ram
          addr = handlePageLoad(addr, offset, false, virtualAddress);
        }
      else {
        addr = tmp_addr;
      }

//      std::cout << "in table " << i << ", offset: " << offset << ". we read" << " addr: " << addr << ", depth: " << i << std::endl;
    }

  uint64_t offset = readBits(virtualAddress, bits_length_start, bits_length_end);
//  std::cout << "bits_length_start: " << bits_length_start << " bits_length_end: " << bits_length_end << ", offset: " << offset << std::endl;
//  std::cout << "bits_length: " << BITS_OF_PT_ADDR << ", at i: " << (DEPTH_OF_PT_TREE-1) << std::endl;

  PMread(addr * PAGE_SIZE + offset ,&tmp_addr);

  if (!tmp_addr) {
      // load page to ram
      addr = handlePageLoad(addr, offset, true, virtualAddress);
    }
  else
    {
      addr = tmp_addr;
    }
//  std::cout << "in table " << 4 << ", offset: " << offset << ". we read" << " data addr: " << addr << ", depth: " << 4 << std::endl;

  uint64_t d = readBits (virtualAddress, VIRTUAL_ADDRESS_WIDTH - D_OFFSET, VIRTUAL_ADDRESS_WIDTH);
  PMread(addr * PAGE_SIZE + d,value);
//  std::cout << "data from: " << addr << ", offset: " << d << ", value: " << *value << ", depth: " << 4 << std::endl;
//  std::cout << "full addr: " << addr * PAGE_SIZE + d << std::endl;
  return 1;
}


