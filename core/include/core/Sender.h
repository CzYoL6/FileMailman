//
// Created by zaiyichen on 2023/11/29.
//

#ifndef IMGUIOPENGL_SENDER_H
#define IMGUIOPENGL_SENDER_H

#include <core/UdpClient.h>

#include <utility>

class Sender : public UdpClient{

public:
    Sender(boost::asio::io_context &io_context, uint16_t port, std::string_view filename);
    ~Sender() override;

protected:
    void handle_data(boost::asio::ip::udp::endpoint endpoint, std::span<char> data) override;
    void send_begin_transfer_ack(const std::string &file_name, int file_size);
    void send_begin_block_ack(int block_id);
    void send_slice_data(int block_id, int slice_id, std::span<char> slice_data);
    void send_end_transfer_ack();

//public:
//    void set_current_buffer_block(std::shared_ptr<BufferBlock> buffer_block) { _current_buffer_block = std::move(buffer_block); }

private:

    void load_block(int id);

    std::shared_ptr<std::ifstream> _ifs;
    uint64_t _file_size;
    uint64_t _block_count;
    std::string _file_name;
    std::unique_ptr<BufferBlock> _current_buffer_block;
    int _current_buffer_block_id{ -1};

    boost::asio::steady_timer _close_wait_timer;

    bool _transferring;


    boost::asio::ip::udp::endpoint _receiver_endpoint;
};


#endif //IMGUIOPENGL_SENDER_H
