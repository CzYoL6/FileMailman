//
// Created by zaiyi on 2023/11/7.
//

#ifndef IMGUIOPENGL_BUFFERBLOCK_H
#define IMGUIOPENGL_BUFFERBLOCK_H

#include <string_view>
#include <vector>
#include <memory>
#include <assert.h>

class BlockSlice;

class BufferBlock {
public:
    BufferBlock(int slice_count, int slice_size);
    ~BufferBlock();

public:
    void ReadFromFile(std::string_view file_path, int begin);
    void WriteToFile(std::string_view file_path, int begin);
private:
    int _slice_count;
    int _slice_size;
    std::vector<std::shared_ptr<BlockSlice>> _slices;
};

class BlockSlice{
public:
    BlockSlice(int size);

    ~BlockSlice();

public:
    void Load(char* bytes, int count);

public:
    int size() const { return _size ;}
private:
    char* _bytes;
    int _size;
};

#endif //IMGUIOPENGL_BUFFERBLOCK_H
