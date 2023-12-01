//
// Created by zaiyichen on 2023/11/30.
//

#include <core/UdpClient.h>
#include <iostream>
#include <core/Sender.h>
#include <spdlog/spdlog.h>
#include <core/Message.h>
#include <fstream>
#include <filesystem>

UdpClient::UdpClient(std::string_view ip, uint16_t port)
    : _io_context(),_socket_write_strand(_io_context),
      _socket(_io_context,
              boost::asio::ip::udp::endpoint(
                      boost::asio::ip::address::from_string(ip.data()),
                      port))
{
    do_receive();
    for(int i = 0; i < thread_pool_size; i++){
        _thread_pool.emplace_back([&]{_io_context.run();});
    }
}
UdpClient::UdpClient(uint16_t port)
    : _io_context(),_socket_write_strand(_io_context),
      _socket(_io_context,
              boost::asio::ip::udp::endpoint (
                      boost::asio::ip::udp::v4(),
                      port
                      ))

{
    do_receive();
    for(int i = 0; i < thread_pool_size; i++){
        _thread_pool.emplace_back([&]{_io_context.run();});
    }
}



UdpClient::~UdpClient() {
    for(auto &i : _thread_pool){
        if(i.joinable()) i.join();
    }
    if(_socket.is_open()){
        _socket.close();
    }
}

void UdpClient::do_receive() {
    _socket.async_receive_from(
            boost::asio::buffer(_receive_data, max_length), _other_endpoint,
            [this](boost::system::error_code ec, std::size_t bytes_recved) {
                if (ec) return;
                spdlog::info("Receive {} bytes of data: .", bytes_recved);
                std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(_receive_data[0])<<std::endl;
                std::string data(_receive_data, _receive_data + bytes_recved);
                _io_context.post(_socket_write_strand.wrap([d =std::move(data), this]mutable {
                    auto x = handle_data(std::span<char>(d.begin(), d.end()));
                    queue_send_data(x);
                }));
                do_receive();
            });
}

void UdpClient::do_send() {
    _socket.async_send_to(
            std::holds_alternative<std::span<char>>(_send_data_deque.front()) ?
            boost::asio::buffer(std::get<std::span<char>>(_send_data_deque.front()), std::get<std::span<char>>(_send_data_deque.front()).size()) :
            boost::asio::buffer(std::get<std::vector<unsigned char>>(_send_data_deque.front()), std::get<std::vector<unsigned char>>(_send_data_deque.front()).size())
            ,
            _other_endpoint,
            _socket_write_strand.wrap([this](const boost::system::error_code& error, std::size_t bytes_transferred){
                send_data_done(error);
            })
    );
}

void UdpClient::queue_send_data(const std::variant<std::span<char>, std::vector<unsigned char>> &data) {
    bool in_progress = !_send_data_deque.empty();
    _send_data_deque.push_back(data);

    if(!in_progress){
        do_send();
    }
}

void UdpClient::send_data_done(const boost::system::error_code &error) {
    if(!error){
        _send_data_deque.pop_front();
        if(!_send_data_deque.empty()) do_send();
    }
}


