#ifndef EX4_CATCHEFSCONTAINER_H
#define EX4_CATCHEFSCONTAINER_H

#include <iostream>
#include <list>
#include <map>
#include <cstring>

#define TMP_DIR "/tmp"

using namespace std;

struct cfile {
    map<off_t, int> blocks_map;
    list<int> fd_list;
    string f_name;
};

class CacheFS_Container {
public:
    CacheFS_Container();
    CacheFS_Container(int amount_of_blocks);

    virtual ~CacheFS_Container();

    int open_file(const char *pathname);
    int close_file(int file_id);
    int fs_pread(int file_id, void *buf, size_t count, off_t offset);
    int print_cache(const char *log_path);
    int print_stat(const char *log_path);

protected:

    char* _read_block(int file_id, off_t offset, ssize_t *count);

    virtual void _handle_not_in_cache(int file_id, off_t offset, char* block) = 0;
    virtual void _handle_in_cache(int file_id, off_t offset) = 0;
    virtual void _print_cache(FILE* fd) = 0;

    char **blocks;
    ssize_t *blocks_size;

    map<std::string, cfile> cfiles;
    map<int, cfile*> fd_to_cfile;
    map<int, cfile*> block_index_to_cfile;

    int *time_stamps;

    int num_of_blocks;
    int block_size;
    int num_of_hits;
    int num_of_misses;
};





#endif //EX4_CATCHEFSCONTAINER_H
