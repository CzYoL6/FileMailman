//
// Created by zaiyichen on 2023/11/9.
//

#ifndef IMGUIOPENGL_FILEMANAGER_H
#define IMGUIOPENGL_FILEMANAGER_H

#include <filesystem>
#include <vector>
#include <core/BufferBlock.h>

class FileManager {
public:
    FileManager();
    ~FileManager();

public:
    void ReadFileIntoBlock(std::string_view file_path, int size);
    void WriteBlockIntoFile(std::string_view file_path);
public:
    void Init(std::string_view tfp, int buffer_block_count, int buffer_block_size, int block_slick_count, int block_slice_size);

public:
    void set_tmp_file_prefix(std::string_view tfp){_temp_file_prefix = tfp;}
    std::string_view temp_file_prefix() const { return _temp_file_prefix; }


public:
    static FileManager& instance() {
        static FileManager instance;
        return instance;
    }

private:
    std::vector<std::shared_ptr<BufferBlock>> _buffer_blocks;
    std::string _temp_file_prefix;
};


#endif //IMGUIOPENGL_FILEMANAGER_H
