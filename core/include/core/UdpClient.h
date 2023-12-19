//
// Created by zaiyichen on 2023/11/30.
//
#include <boost/asio.hpp>
#include <memory>
#include <vector>
#include <deque>
#include <core/BufferBlock.h>
#include <iostream>

#ifndef IMGUIOPENGL_UDPCLIENT_H
#define IMGUIOPENGL_UDPCLIENT_H


class UdpClient : public std::enable_shared_from_this<UdpClient> {

public:
    enum { max_length = 2048, thread_pool_size = 8, block_size = 1024 * 8, slice_size = 1024};
    explicit UdpClient(std::string_view ip, uint16_t port);
    explicit UdpClient(uint16_t port);
    explicit UdpClient();
    virtual ~UdpClient();
    void Start();

protected:
    void wait_for_all_tasks();
    void queue_send_data(const boost::asio::ip::udp::endpoint& otherend, std::vector<unsigned char> &data);
    void do_receive();
    void do_send();
    virtual void handle_data(boost::asio::ip::udp::endpoint endpoint, std::span<char> data) =0;
    void send_data_done(const boost::system::error_code& error);

protected:
    boost::asio::io_context _io_context;
    boost::asio::ip::udp::socket _socket;
    boost::asio::io_context::strand _socket_write_strand;
    char _receive_data[max_length]{};
    std::vector<std::thread> _thread_pool;
    std::deque<std::pair<boost::asio::ip::udp::endpoint ,std::vector<unsigned char>>> _send_data_deque;
    bool _running{false};

};


#endif //IMGUIOPENGL_UDPCLIENT_H
