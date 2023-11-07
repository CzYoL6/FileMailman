//
// Created by zaiyi on 2023/11/7.
//

#include <core/BufferBlock.h>

BlockSlice::BlockSlice(int size) : _size(size){
    _bytes = new char[size];
}

BlockSlice::~BlockSlice() {
    delete[] _bytes;
}

void BlockSlice::Load(char *bytes, int count) {
    assert(count == _size);

    memcpy(_bytes, bytes, count);
}

BufferBlock::BufferBlock(int slice_count, int slice_size) : _slice_count(slice_count), _slice_size(slice_size){

}

BufferBlock::~BufferBlock() {

}

void BufferBlock::ReadFromFile(std::string_view file_path, int begin) {
    
}

void BufferBlock::WriteToFile(std::string_view file_path, int begin) {

}
