//
// Created by zaiyichen on 2023/11/29.
//

#ifndef IMGUIOPENGL_RECEIVER_H
#define IMGUIOPENGL_RECEIVER_H

#include <boost/asio.hpp>
#include <deque>
#include<vector>
#include <core/BufferBlock.h>
#include <core/UdpClient.h>
#include <functional>
#include <unordered_map>
#include <algorithm>

//template<> struct std::hash<std::pair<int,int>>{
//size_t operator()(const std::pair<int,int>& v) const{
//    return v.first ^ v.second;
//}
//};
//
//template<>
//struct std::equal_to<std::pair<int,int>>{
//    bool operator()(const std::pair<int,int>& a, const std::pair<int,int>& b) const{
//        return a.first == b.first && a.second == b.second;
//    }
//};


enum { max_length = 1024 , thread_pool_count = 8, slice_size = 1024, block_size = 1024 * 8};
class Receiver : public UdpClient{

public:
    Receiver(boost::asio::io_context &io_context, std::string_view ip, uint16_t port);
    ~Receiver() override;

protected:
    HandleDataRetValue
    handle_data(boost::asio::ip::udp::endpoint endpoint, std::span<char> data) override;


private:
    void save_block(int id);

    Receiver::HandleDataRetItem generate_begin_transfer();
    Receiver::HandleDataRetItem generate_begin_block(int block_id);
    Receiver::HandleDataRetItem
    generate_require_slice(int block_id, int slice_id);
    Receiver::HandleDataRetItem generate_end_transfer();



private:
    std::shared_ptr<std::ofstream> _ofs;
    uint64_t _file_size;
    uint64_t _block_count;
    std::string _file_name;
    int _current_block_id{ -1};
    int _current_block_slice_id{ -1};
    std::unique_ptr<BufferBlock> _current_buffer_block;
    boost::asio::ip::udp::endpoint _sender_endpoint;

    std::chrono::steady_clock _total_time_timer;
    std::chrono::time_point<std::chrono::steady_clock> _time_point;
};


#endif //IMGUIOPENGL_RECEIVER_H
