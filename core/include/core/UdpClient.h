//
// Created by zaiyichen on 2023/11/30.
//
#include <boost/asio.hpp>
#include <memory>
#include <vector>
#include <deque>
#include <core/BufferBlock.h>

#ifndef IMGUIOPENGL_UDPCLIENT_H
#define IMGUIOPENGL_UDPCLIENT_H


class UdpClient {

public:
    enum { max_length = 1024, thread_pool_size = 8, block_size = 1024 * 8, slice_size = 1024};
    explicit UdpClient(std::string_view ip, uint16_t port);
    explicit UdpClient(uint16_t port);
    virtual ~UdpClient();

protected:
    void do_receive();
    void do_send();
    virtual std::variant<std::span<char>, std::vector<unsigned char>> handle_data(std::span<char> data) = 0;
    void queue_send_data(const std::variant<std::span<char>, std::vector<unsigned char>> &data);
    void send_data_done(const boost::system::error_code& error);

protected:
    boost::asio::io_context _io_context;
    boost::asio::ip::udp::socket _socket;
    boost::asio::ip::udp::endpoint _other_endpoint;
    boost::asio::io_context::strand _socket_write_strand;
    char _receive_data[max_length];
    std::vector<std::thread> _thread_pool;
    std::deque<std::variant<std::span<char>, std::vector<unsigned char>>> _send_data_deque;


};


#endif //IMGUIOPENGL_UDPCLIENT_H
