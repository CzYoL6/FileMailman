//
// Created by zaiyichen on 2023/11/29.
//

#ifndef IMGUIOPENGL_CLIENT_H
#define IMGUIOPENGL_CLIENT_H

#include <boost/asio.hpp>
#include <deque>
#include<vector>

class Client {
    enum { max_length = 1024 , thread_pool_count = 8};

public:
    Client(std::string_view ip, uint16_t port, int block_id);
public:

private:
    void do_receive();
    void do_send();
    void handle_data();
    void queue_send_data();
    void send_data_done(const boost::system::error_code& error);

private:
    boost::asio::io_context _io_context;
    boost::asio::ip::udp::socket _socket;
    boost::asio::ip::udp::endpoint _server_endpoint;
    boost::asio::io_context::strand _socket_write_strand;
    char _receive_data[max_length];
    std::vector<std::thread> _thread_pool;
    std::deque<std::span<char>> _send_data_deque;

private:
    int _current_block_id;
    int _current_block_slice_id;
};


#endif //IMGUIOPENGL_CLIENT_H
