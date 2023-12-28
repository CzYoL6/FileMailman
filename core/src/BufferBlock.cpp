//
// Created by zaiyi on 2023/11/7.
//

#include<fstream>
#include <iostream>
#include <core/BufferBlock.h>
#include <core/Utils.hpp>



BlockSlice::BlockSlice(BufferBlock &buffer_block, int slice_id, char *begin, int bytes_size)
    : _buffer_block(buffer_block), _slice_id(slice_id),  _data_span(begin, bytes_size){
}

void BlockSlice::ReadOut(char *begin, int count) {
    assert(count == _data_span.size());
    memcpy(begin, _data_span.data(), count);
}

void BlockSlice::WriteIn(char *begin, int count) {
    assert(count == _data_span.size());
    memcpy(_data_span.data(), begin, count);
}

BlockSlice::~BlockSlice() {

}



BufferBlock::~BufferBlock() {
    delete[] _data;
}

void BufferBlock::ReadFromFile(std::ifstream &ifs, int begin_bytes, int count) {
    assert(ifs.is_open());
    std::lock_guard<std::mutex> lock(_mutex);
    ifs.seekg(begin_bytes, std::ios::beg);
    ifs.read(_data, count);

}

void BufferBlock::WriteToFile(std::ofstream &ofs, int begin_bytes, int count) {
    assert(ofs.is_open());
    std::lock_guard<std::mutex> lock(_mutex);
    ofs.seekp(begin_bytes, std::ios::beg);
    ofs.write(_data,count);

}

void BufferBlock::SplitIntoSlices() {
    for(int i = 0; i < _bytes_size; i += _slice_size){
        _slices.emplace_back(std::make_shared<BlockSlice>(*this,
                                                          _slice_count,
                                                          _data + i,
                                                          std::min(_slice_size, (int)(_bytes_size - _slice_count * _slice_size))));
        _slice_count++;
    }

}

BufferBlock::BufferBlock(int block_id, int slice_bytes_size, int bytes_size)
        : _block_id(block_id),
          _slice_size(slice_bytes_size),
          _slice_count(0),
          _bytes_size(bytes_size){
    _data = new char[bytes_size];
    memset(_data, 0, sizeof(_data));
    SplitIntoSlices();
}

bool BufferBlock::IsSliceDone(int id) {
    std::lock_guard<std::mutex> lock(_mutex);
    return (_bitmap & (1 << id)) > 0;
}

void BufferBlock::SetSliceDone(int id) {
    std::lock_guard<std::mutex> lock(_mutex);
    _bitmap |= (1 << id);
}

bool BufferBlock::SliceAllDone() {
    std::lock_guard<std::mutex> lock(_mutex);
    for(int i = 0; i < _slice_count; i++){
        if((_bitmap & (1 << i)) == 0) return false;
    }
    return true;
}
