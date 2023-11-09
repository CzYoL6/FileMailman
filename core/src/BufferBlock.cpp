//
// Created by zaiyi on 2023/11/7.
//

#include <core/BufferBlock.h>


BlockSlice::BlockSlice(std::vector<unsigned char>::iterator begin, int count) {
    Read(begin, count);
}

void BlockSlice::Read(std::vector<unsigned char>::iterator begin, int count) {
    assert(count == _size);
    _data_span = std::span<unsigned char>(begin, begin + count);
}

void BlockSlice::Write(unsigned char *bytes, int count) {
    assert(count == _data_span.size());
    memcpy(_data_span.data(), bytes, count);
}

BlockSlice::~BlockSlice() {

}


BufferBlock::BufferBlock(int slice_count, int slice_size) : _slice_count(slice_count), _slice_size(slice_size){

}

BufferBlock::~BufferBlock() {

}

void BufferBlock::ReadFromFile(std::string_view file_path, int begin) {
    
}

void BufferBlock::WriteToFile(std::string_view file_path, int begin) {

}
