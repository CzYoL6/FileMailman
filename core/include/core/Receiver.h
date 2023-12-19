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

    enum { max_length = 1024 , thread_pool_count = 8, slice_size = 1024, block_size = 1024 * 8};
class Receiver : public UdpClient{

public:
    Receiver(boost::asio::io_context &io_context, std::string_view ip, uint16_t port);
    ~Receiver() override;

protected:
    void handle_data(boost::asio::ip::udp::endpoint endpoint, std::span<char> data) override;

private:
    void save_block(int id);

    void send_begin_transfer();
    void send_begin_block(int block_id);
    void send_require_slice(int block_id, int slice_id);
    void send_end_transfer();

private:
    std::shared_ptr<std::ofstream> _ofs;
    uint64_t _file_size;
    uint64_t _block_count;
    std::string _file_name;
    int _current_block_id{ -1};
    int _current_block_slice_id{ -1};
    std::unique_ptr<BufferBlock> _current_buffer_block;
    boost::asio::ip::udp::endpoint _sender_endpoint;

};


#endif //IMGUIOPENGL_RECEIVER_H
