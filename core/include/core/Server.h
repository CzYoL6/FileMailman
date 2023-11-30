//
// Created by zaiyichen on 2023/11/29.
//

#ifndef IMGUIOPENGL_SERVER_H
#define IMGUIOPENGL_SERVER_H

#include <boost/asio.hpp>
#include <memory>
#include <vector>
#include <deque>
#include <core/BufferBlock.h>

class Server{
    enum { max_length = 1024, thread_pool_size = 8, block_size = 1024 * 8, slice_size = 1024};
    enum State{ kWaiting, kReady, kTransferring};


public:
    Server(uint16_t port, std::string_view filename);
    ~Server();

private:
    void do_receive();
    void do_send();
    std::variant<std::span<char>, std::vector<unsigned char>> handle_data(std::span<char> data);
    void queue_send_data(const std::variant<std::span<char>, std::vector<unsigned char>> &data);
    void send_data_done(const boost::system::error_code& error);

    void load_block(int id);

public:
    int file_size() const { return _file_size;}
    std::string_view file_name() const {return _file_name;}

private:
    boost::asio::io_context _io_context;
    boost::asio::ip::udp::socket _socket;
    boost::asio::ip::udp::endpoint _client_endpoint;
    boost::asio::io_context::strand _socket_write_strand;
    char _receive_data[max_length];
    std::vector<std::thread> _thread_pool;
    std::deque<std::variant<std::span<char>, std::vector<unsigned char>>> _send_data_deque;

    std::unique_ptr<std::ifstream> _ifs;
    uint64_t _file_size;
    uint64_t _block_count;
    std::string _file_name;
    std::unique_ptr<BufferBlock> _current_buffer_block;
    int _current_buffer_block_id{-1};

    boost::asio::steady_timer _close_wait_timer;
};


#endif //IMGUIOPENGL_SERVER_H
