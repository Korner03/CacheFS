//
// Created by cabby333 on 6/4/17.
//
/*
 * if (read_size - start < expected_length)
 * expected_length = read_size - start
 */
#ifndef EX4_CACHFSALGOS_H
#define EX4_CACHFSALGOS_H

#include <vector>
#include <deque>
#include "CacheFSContainer.h"

class LRU_Container : public CacheFS_Container {
public:

    LRU_Container(int amount_of_blocks): CacheFS_Container(amount_of_blocks) {
        block_counter = new int[num_of_blocks]();

        counter = 1;

    }

    ~LRU_Container() {
        delete[] block_counter;
    }

protected:
    void _handle_not_in_cache(int file_id, off_t offset, char* block);
    void _handle_in_cache(int file_id, off_t offset);
    void _print_cache(FILE* fd);

private:
    int *block_counter;
    int counter;
};

class LFU_Container : public CacheFS_Container {
public:
    LFU_Container(int amount_of_blocks): CacheFS_Container(amount_of_blocks) {
        block_counter = new int[num_of_blocks]();
    }

    ~LFU_Container() {
        delete[] block_counter;
    }

protected:
    void _handle_not_in_cache(int file_id, off_t offset, char* block);
    void _handle_in_cache(int file_id, off_t offset);
    void _print_cache(FILE* fd);

private:
        int *block_counter;
};


class FBR_Container : public CacheFS_Container {
public:
    FBR_Container(double f_old, double f_new, int amount_of_blocks):
            CacheFS_Container(amount_of_blocks) {

        for (int i = 0; i < num_of_blocks; i++) {
            block_queue.push_back(make_pair(i,-1));
        }

        old_index = (int) (f_old * num_of_blocks);
        new_index = (int) (f_new * num_of_blocks);

    }

    ~FBR_Container() {
        block_queue.clear();
    }

protected:
    void _handle_not_in_cache(int file_id, off_t offset, char* block);
    void _handle_in_cache(int file_id, off_t offset);
    void _print_cache(FILE* fd);

    deque<pair<int,int>> block_queue; // <block_id, counter>
    int old_index;
    int new_index;
};

#endif //EX4_CACHFSALGOS_H
