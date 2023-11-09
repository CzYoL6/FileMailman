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
    BufferBlock(int slice_size);
    ~BufferBlock();

public:
    void ReadFromFile(std::ifstream &ifs, int begin);
    void WriteToFile(std::ofstream& ofs, int begin);

private:
    void SplitIntoSlices();

public:
    char* data()  {return _data.data();}

private:
    int _slice_count;
    int _slice_size;
    std::vector<std::shared_ptr<BlockSlice>> _slices;
    std::vector<char> _data;
};

class BlockSlice{
public:
    BlockSlice(std::vector<char>::iterator begin, int count);

    ~BlockSlice();

public:
    void Read(std::vector<char>::iterator begin, int count);
    void Write(char* bytes, int count);

public:
    char* data() const {return _data_span.data();}
private:
    std::span<char> _data_span;
};

#endif //IMGUIOPENGL_BUFFERBLOCK_H
