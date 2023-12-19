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
#include <boost/asio.hpp>

class BlockSlice;

class BufferBlock {
public:
    BufferBlock(int block_id, int slice_bytes_size, int bytes_size, boost::asio::io_context &io_context);
    ~BufferBlock();

public:
    void ReadFromFile(std::ifstream &ifs, int begin_bytes, int count);
    void WriteToFile(std::ofstream &ofs, int begin_bytes, int count);
    bool IsSliceDone(int id);
    void SetSliceDone(int id);
    bool SliceAllDone();

private:
    void SplitIntoSlices();

public:
    char* data()  {return _data;}
    int size() const{return _bytes_size;}
    auto GetBlockSlice(int id){return _slices[id];}
    int slice_count() const {return _slice_count;}
    int block_id() const {return _block_id;}
    void set_block_id(int v) { _block_id = v;}

private:
    int _slice_count{0};
    int _slice_size;
    std::vector<std::shared_ptr<BlockSlice>> _slices;
    char* _data;
    int _bytes_size;
    int _bitmap{0};
    boost::asio::io_context& _io_context;
    int _block_id;
};

class BlockSlice{
public:
    BlockSlice(int slice_id, char *begin, int bytes_size, boost::asio::io_context &io_context);

    ~BlockSlice();

public:
    void ReadOut(char *begin, int count);
    void WriteIn(char* begin, int count);
    void StartTimeoutTimer(int msec, std::function<void(const boost::system::error_code&)> function);

public:
    int retry_times() const{return _retry_times;}
    void set_retry_times(int v) { _retry_times = v;}

public:
    char* data() const {return _data_span.data();}
    std::span<char> data_span() {return _data_span;}
    int slice_id() const { return _slice_id;}
    void set_slice_id(int v) { _slice_id = v;}

private:
    std::span<char> _data_span;
    boost::asio::steady_timer _timeout_timer;
    int _retry_times{0};
    enum{max_retry_times = 5};
    boost::asio::io_context& _io_context;
    int _slice_id;
};

#endif //IMGUIOPENGL_BUFFERBLOCK_H
