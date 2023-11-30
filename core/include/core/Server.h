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
    enum { max_length = 1024, thread_pool_size = 16};
    enum State{ kWaiting, kReady, kTransferring};


public:
    Server(uint16_t port);

private:
    void do_receive();
    void do_send();
    static std::variant<std::span<char>, std::vector<unsigned char>> handle_data(std::span<char> data);
    void queue_send_data(const std::variant<std::span<char>, std::vector<unsigned char>> &data);
    void send_data_done(const boost::system::error_code& error);

public:
    State state() const {return _state;}

private:
    boost::asio::io_context _io_context;
    boost::asio::ip::udp::socket _socket;
    boost::asio::ip::udp::endpoint _client_endpoint;
    boost::asio::io_context::strand _socket_write_strand;
    char _receive_data[max_length];
    std::vector<std::thread> _thread_pool;
    std::deque<std::variant<std::span<char>, std::vector<unsigned char>>> _send_data_deque;
    State _state{State::kWaiting};

};


#endif //IMGUIOPENGL_SERVER_H
