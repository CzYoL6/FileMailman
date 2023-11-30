//
// Created by zaiyichen on 2023/11/29.
//

#ifndef IMGUIOPENGL_SERVER_H
#define IMGUIOPENGL_SERVER_H

#include <boost/asio.hpp>
#include <memory>
#include <vector>
#include <deque>

class Server{
public:
    Server(uint16_t port);

private:
    void do_receive();
    void do_send();
    std::span<char> handle_data(std::size_t length);
    void queue_send_data(std::span<char> data);
    void send_data_done(const boost::system::error_code& error);

private:
    boost::asio::io_context _io_context;
    boost::asio::ip::udp::socket _socket;
    boost::asio::ip::udp::endpoint _client_endpoint;
    boost::asio::io_context::strand _socket_write_strand;
    enum { max_length = 1024, thread_pool_size = 16 };
    char _receive_data[max_length];
    std::vector<std::thread> _thread_pool;
    std::deque<std::span<char>> _send_data_deque;
};


#endif //IMGUIOPENGL_SERVER_H
