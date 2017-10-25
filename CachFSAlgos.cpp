//
// Created by cabby333 on 6/4/17.
//

#include <algorithm>
#include "CachFSAlgos.h"

/* ------------------------------------------------------------------
 * LRU
 * ----------------------------------------------------------------*/
void LRU_Container::_handle_not_in_cache(int file_id, off_t offset, char* block) {

    // find the block who was least recently used
    int lowest = block_counter[0];
    int lowest_block_id = 0;

    for (int i = 1; i < num_of_blocks; i++) {
        if (block_counter[i] < lowest) {
            lowest = block_counter[i];
            lowest_block_id = i;
        }
    }

    // discard
    if (lowest != 0) {
        int old_offset = 0;
        for (auto &kvp : block_index_to_cfile[lowest_block_id]->blocks_map) {
            if (kvp.second == lowest_block_id) {
                old_offset = (int) kvp.first;
                break;
            }
        }
        block_index_to_cfile[lowest_block_id]->blocks_map.erase(old_offset);

        free(blocks[lowest_block_id]);
    }

    // replace
    blocks[lowest_block_id] = block;
    block_counter[lowest_block_id] = counter;
    counter++;

    // set the maps
    fd_to_cfile[file_id]->blocks_map[offset] = lowest_block_id;
    block_index_to_cfile[lowest_block_id] = fd_to_cfile[file_id];

}

void LRU_Container::_handle_in_cache(int file_id, off_t offset) {

    block_counter[fd_to_cfile[file_id]->blocks_map[offset]] = counter;
    counter++;

}


void LRU_Container::_print_cache(FILE *fd) {

    vector<pair<int,int>> to_print_vec;
    for (int i = 0; i < num_of_blocks; ++i) {

        if (block_counter[i] != 0)
            to_print_vec.push_back(make_pair(i, block_counter[i])); // (block_id, recent_use)

    }

    // sorting by recent use
    sort(to_print_vec.begin(), to_print_vec.end(),
         [](const pair<int, int> &left, const pair<int, int> &right)
         {
             return left.second > right.second;
         });

    for (auto& kvp : to_print_vec) {

        off_t offset = 0;
        for (auto& obp : block_index_to_cfile[kvp.first]->blocks_map) {
            if (obp.second == kvp.first) {
                offset = obp.first;
                break;
            }
        }

        fprintf(fd, "%s %ld\n",
                block_index_to_cfile[kvp.first]->f_name.c_str(),
                offset / block_size
        );

    }

}

/* ------------------------------------------------------------------
 * LFU
 * ----------------------------------------------------------------*/
void LFU_Container::_handle_not_in_cache(int file_id, off_t offset, char* block) {

    // find the block who was most recently used
    int lowest = block_counter[0];
    int lowest_block_id = 0;

    for (int i = 1; i < num_of_blocks; i++) {
        if (block_counter[i] < lowest) {
            lowest = block_counter[i];
            lowest_block_id = i;
        }
    }

    // discard
    if (lowest != 0) {
        int old_offset = 0;
        for (auto &kvp : block_index_to_cfile[lowest_block_id]->blocks_map) {
            if (kvp.second == lowest_block_id) {
                old_offset = (int) kvp.first;
                break;
            }
        }
        block_index_to_cfile[lowest_block_id]->blocks_map.erase(old_offset);
    }
    free(blocks[lowest_block_id]);

    // replace
    blocks[lowest_block_id] = block;
    block_counter[lowest_block_id] = 1;

    // set the maps
    fd_to_cfile[file_id]->blocks_map[offset] = lowest_block_id;
    block_index_to_cfile[lowest_block_id] = fd_to_cfile[file_id];

}

void LFU_Container::_handle_in_cache(int file_id, off_t offset) {
    block_counter[fd_to_cfile[file_id]->blocks_map[offset]]++;
}

void LFU_Container::_print_cache(FILE *fd) {

    vector<pair<int,int>> to_print_vec;
    for (int i = 0; i < num_of_blocks; ++i) {

        if (block_counter[i] != 0)
            to_print_vec.push_back(make_pair(i, block_counter[i])); // (block_id, recent_use)

    }

    // sorting by freq use
    sort(to_print_vec.begin(), to_print_vec.end(),
         [](const pair<int, int> &left, const pair<int, int> &right)
         {
             return left.second > right.second;
         });

    for (auto& kvp : to_print_vec) {

        off_t offset = 0;
        for (auto& obp : block_index_to_cfile[kvp.first]->blocks_map) {
            if (obp.second == kvp.first) {
                offset = obp.first;
                break;
            }
        }

        fprintf(fd, "%s %ld\n",
                block_index_to_cfile[kvp.first]->f_name.c_str(),
                offset / block_size
        );

    }
}

/* ------------------------------------------------------------------
 * FBR
 * ----------------------------------------------------------------*/
void FBR_Container::_handle_not_in_cache(int file_id, off_t offset, char* block) {

    int lowest_index = old_index;
    int lowest_block_id = block_queue[lowest_index].first;
    int lowest_frequency = block_queue[lowest_index].second;

    for (int i = lowest_index + 1; i < num_of_blocks; i++) {
        if (block_queue[i].second <= lowest_frequency) {
            lowest_index = i;
            lowest_block_id = block_queue[lowest_index].first;
            lowest_frequency = block_queue[lowest_index].second;
        }
    }

    // discard
    if (lowest_frequency != -1) {
        int old_offset = 0;
        for (auto &kvp : block_index_to_cfile[lowest_block_id]->blocks_map) {
            if (kvp.second == lowest_block_id) {
                old_offset = (int) kvp.first;
                break;
            }
        }
        block_index_to_cfile[lowest_block_id]->blocks_map.erase(old_offset);

        free(blocks[lowest_block_id]);
    }

    // replace
    block_queue.erase(block_queue.begin() + lowest_index);
    block_queue.push_front(make_pair(lowest_block_id, 1));
    blocks[lowest_block_id] = block;

    // set the maps
    fd_to_cfile[file_id]->blocks_map[offset] = lowest_block_id;
    block_index_to_cfile[lowest_block_id] = fd_to_cfile[file_id];
}

void FBR_Container::_handle_in_cache(int file_id, off_t offset) {

    int block_id = fd_to_cfile[file_id]->blocks_map[offset];
    int new_counter;
    int index_found = -1;

    // find block id in the queue.
    for (int i = 0; i < num_of_blocks; ++i) {
        if (block_queue[i].first == block_id) {
            new_counter = block_queue[i].second;

            if (i >= new_index)
                new_counter++;

            index_found = i;
            break;
        }
    }

    // exit if not found (not possible..)
    if (index_found == -1) {
        cerr << "error\n" << endl;
        exit(1);
    }

    // remove from queue and requeue.
    block_queue.erase(block_queue.begin() + index_found);
    block_queue.push_front(make_pair(block_id, new_counter));
}


void FBR_Container::_print_cache(FILE *fd) {

    vector<int> to_print_vec;
    // pushing block id's into vector according to LRU order
    for (auto& pair : block_queue) {

        if (pair.second != -1)
            to_print_vec.push_back(pair.first);

    }

    for (auto& block_index : to_print_vec) {

        off_t offset = 0;
        for (auto& obp : block_index_to_cfile[block_index]->blocks_map) {
            if (obp.second == block_index) {
                offset = obp.first;
                break;
            }
        }

        fprintf(fd, "%s %ld\n",
                block_index_to_cfile[block_index]->f_name.c_str(),
                offset / block_size
        );

    }

}