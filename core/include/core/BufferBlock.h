//
// Created by zaiyi on 2023/11/7.
//

#ifndef IMGUIOPENGL_BUFFERBLOCK_H
#define IMGUIOPENGL_BUFFERBLOCK_H

#include <string_view>
#include <vector>
#include <memory>
#include <assert.h>
#include <span>
#include <vector>

class BlockSlice;

class BufferBlock {
public:
    BufferBlock(int slice_count, int slice_size);
    ~BufferBlock();

public:
    void ReadFromFile(std::string_view file_path, int begin);
    void WriteToFile(std::string_view file_path, int begin);

public:
    unsigned char* data()  {return _data.data();}

private:
    int _slice_count;
    int _slice_size;
    std::vector<std::shared_ptr<BlockSlice>> _slices;
    std::vector<unsigned char> _data;
};

class BlockSlice{
public:
    BlockSlice(std::vector<unsigned char>::iterator begin, int count);

    ~BlockSlice();

public:
    void Read(std::vector<unsigned char>::iterator begin, int count);
    void Write(unsigned char* bytes, int count);

public:
    unsigned char* data() const {return _data_span.data();}
private:
    std::span<unsigned char> _data_span;
};

#endif //IMGUIOPENGL_BUFFERBLOCK_H
