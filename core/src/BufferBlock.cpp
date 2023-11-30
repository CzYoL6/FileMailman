//
// Created by zaiyi on 2023/11/7.
//

#include<fstream>
#include <core/BufferBlock.h>



BlockSlice::BlockSlice(char *begin, int bytes_size, boost::asio::io_context &io_context)
    : _io_context(io_context), _timeout_timer(io_context){
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

void BlockSlice::StartTimeoutTimer(int msec, std::function<void(const boost::system::error_code &)> function) {
    _timeout_timer.expires_after(std::chrono::milliseconds (msec));
    _timeout_timer.async_wait(std::move(function));
}


BufferBlock::~BufferBlock() {
    delete[] _data;
}

void BufferBlock::ReadFromFile(std::ifstream &ifs, int begin_bytes, int count) {
    assert(ifs.is_open());
    ifs.seekg(begin_bytes, std::ios::beg);
    ifs.read(_data, count);
}

void BufferBlock::WriteToFile(std::ofstream &ofs, int begin_bytes, int count) {
    assert(ofs.is_open());
    ofs.seekp(begin_bytes, std::ios::beg);
    ofs.write(_data,count);
}

void BufferBlock::SplitIntoSlices() {
    for(int i = 0; i < _bytes_size; i += _slice_size){
        _slices.emplace_back(std::make_shared<BlockSlice>(_data + i,
                                                          std::min(_slice_size, (int)(_bytes_size - _slice_count * _slice_size)),
                                                          _io_context));
        _slice_count++;
    }

}

BufferBlock::BufferBlock(int slice_bytes_size, int bytes_size, boost::asio::io_context &io_context)
        : _slice_size(slice_bytes_size),
          _slice_count(0),
          _bytes_size(bytes_size),
          _io_context(io_context){
    _data = new char[bytes_size];
    SplitIntoSlices();
}

bool BufferBlock::IsSliceDone(int id) {
    return (_bitmap & (1 << id)) > 0;
}

void BufferBlock::SetSliceDone(int id) {
    _bitmap |= (1 << id);
}

bool BufferBlock::SliceAllDone() {
    for(int i = 0; i < _slice_count; i++){
        if((_bitmap & (1 << i)) == 0) return false;
    }
    return true;
}
