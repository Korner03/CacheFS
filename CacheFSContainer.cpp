#include <sys/stat.h>
#include "CacheFSContainer.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>


CacheFS_Container::CacheFS_Container() {}

CacheFS_Container::CacheFS_Container(int num_of_blocks):
        num_of_blocks(num_of_blocks), num_of_hits(0), num_of_misses(0)
{
    struct stat fi;
    stat(TMP_DIR, &fi);
    block_size = fi.st_blksize;

    try {
        blocks = new char *[num_of_blocks]();

        time_stamps = new int[num_of_blocks]();

        blocks_size = new ssize_t[num_of_blocks]();
        for (int i = 0; i < num_of_blocks; ++i) {
            blocks_size[i] = 0;
        }

    } catch (bad_alloc &e) {
        exit(1);
    }
}

CacheFS_Container::~CacheFS_Container() {

    for (int i = 0; i < num_of_blocks; i++)
        free(blocks[i]);

    delete[] blocks;
    delete[] time_stamps;
    delete[] blocks_size;
}

int CacheFS_Container::open_file(const char *pathname) {

    int fd = open(pathname, O_RDONLY | O_SYNC | O_DIRECT);

    if (fd == -1) {
        return -1;
    }

    if (cfiles.count(pathname) == 0) {
        cfiles[pathname] = cfile();
        cfiles[pathname].f_name = pathname;
    }

    cfiles[pathname].fd_list.push_back(fd);
    fd_to_cfile[fd] = &cfiles[pathname];


    return fd;
}

int CacheFS_Container::close_file(int file_id) {

    if (fd_to_cfile.count(file_id) == 0) {
        return -1;
    }


    cfile *file = fd_to_cfile[file_id];
    file->fd_list.remove(file_id);
    fd_to_cfile.erase(file_id);


    return 0;
}

int CacheFS_Container::fs_pread(int file_id, void *buf, size_t count, off_t offset) {
    if (fd_to_cfile.find(file_id) == fd_to_cfile.end() || buf == nullptr ||
            offset < 0) {
        return -1;
    }

    if (count == 0)
        return 0;

    int first_block_offset = ((int)offset / block_size) * block_size;
    int first_char_offset = (int)offset - first_block_offset;
    int end_offset = (int)offset + (int)count - 1;
    int last_block_offset = (end_offset / block_size) * block_size;
    int last_char_offset = end_offset - last_block_offset;


    char *block;
    ssize_t read_size = 0;
    int curr_buff_offset = 0;
    for (int i = first_block_offset; i <= last_block_offset; i+=block_size) {

        block = _read_block(file_id, (off_t) i, &read_size);

        int start = (first_block_offset == i) ? first_char_offset : 0;
        int end = (last_block_offset == i) ? last_char_offset : block_size - 1;

        int expected_length = end - start + 1;

        if (read_size != 0 && (read_size - start < expected_length)) {
            expected_length = read_size - start;
        }

        if (read_size < expected_length) {
            memcpy((char*) buf + curr_buff_offset, block + start, (size_t) read_size);
            curr_buff_offset += read_size;

            if (read_size == 0)
                free(block);

            break;

        } else {
            memcpy((char*) buf + curr_buff_offset, block + start, (size_t) expected_length);
            curr_buff_offset += expected_length;
        }

    }

    return curr_buff_offset;
}


char* CacheFS_Container::_read_block(int file_id, off_t offset, ssize_t *count) {

    char* block = nullptr;

    if (fd_to_cfile[file_id]->blocks_map.count(offset) == 0) {

        // read from file (fd) from offset
        block = (char*) aligned_alloc(block_size, block_size);

        if (block == nullptr) {
            exit(1);
        }

        *count = pread(file_id, block, (size_t) block_size, offset);

        if (*count > 0) {
            _handle_not_in_cache(file_id, offset, block);
            blocks_size[fd_to_cfile[file_id]->blocks_map[offset]] = *count;
            num_of_misses++;
        }

    } else {

        block = blocks[fd_to_cfile[file_id]->blocks_map[offset]];
        *count = blocks_size[fd_to_cfile[file_id]->blocks_map[offset]];

        _handle_in_cache(file_id, offset);
        num_of_hits++;
    }
    return block;
}

int CacheFS_Container::print_cache(const char *log_path) {

    FILE* fd = fopen(log_path, "a");
    if (fd == NULL) {
        return -1;
    }
    _print_cache(fd);
    fclose(fd);

    return 0;
}

int CacheFS_Container::print_stat(const char *log_path) {
    if (log_path == nullptr) {
        return -1;
    }

    FILE* fd = fopen(log_path, "a");
    if (fd == NULL) {
        return -1;
    }

    fprintf(fd, "Hits number: %d\nMisses number: %d\n", num_of_hits, num_of_misses);
    fclose(fd);

    return 0;
}
