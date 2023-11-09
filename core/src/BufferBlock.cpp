//
// Created by zaiyi on 2023/11/7.
//

#include<fstream>
#include <core/BufferBlock.h>



BlockSlice::BlockSlice(std::vector<char>::iterator begin, int count) {
    Read(begin, count);
}

void BlockSlice::Read(std::vector<char>::iterator begin, int count) {
    assert(count == _data_span.size());
    _data_span = std::span<char>(begin, begin + count);
}

void BlockSlice::Write(char *bytes, int count) {
    assert(count == _data_span.size());
    memcpy(_data_span.data(), bytes, count);
}

BlockSlice::~BlockSlice() {

}


BufferBlock::~BufferBlock() {

}

void BufferBlock::ReadFromFile(std::ifstream &ifs, int begin) {
    assert(ifs.is_open());

    ifs.read(_data.data(), _slice_size);
}

void BufferBlock::WriteToFile(std::ofstream &ofs, int begin) {
    assert(ofs.is_open());

    ofs.write(_data.data(), _data.size());
}

void BufferBlock::SplitIntoSlices() {
    for(int i = 0; i < _data.size(); i += _slice_size){
        _slices.emplace_back(std::make_shared<BlockSlice>(_data.begin() + i, std::min(_slice_size, (int)(_data.size() - _slice_count * _slice_size))));
        _slice_count++;
    }

}

BufferBlock::BufferBlock(int slice_size) : _slice_size(slice_size), _slice_count(0){

}
