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
#include <core/Utils.hpp>

UdpClient::UdpClient(std::string_view ip, uint16_t port)
    : _io_context(),_socket_write_strand(_io_context),
      _socket(_io_context,
              boost::asio::ip::udp::endpoint(
                      boost::asio::ip::address::from_string(ip.data()),
                      port))
{
}
UdpClient::UdpClient(uint16_t port)
    : _io_context(),_socket_write_strand(_io_context),
      _socket(_io_context,
              boost::asio::ip::udp::endpoint (
                      boost::asio::ip::udp::v4(),
                      port
                      ))

{

}

UdpClient::UdpClient()
    :_io_context(),_socket_write_strand(_io_context),
     _socket(_io_context,boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4::any(), 0))
{
}




UdpClient::~UdpClient() {
    if(_socket.is_open()){
        _socket.close();
    }
}

void UdpClient::do_receive() {
    if(!_running) return;
    boost::asio::ip::udp::endpoint  other_end;
    _socket.async_receive_from(
            boost::asio::buffer(_receive_data, max_length), other_end,
            [this, &other_end](boost::system::error_code ec, std::size_t bytes_recved) {
                if (ec) return;
                spdlog::info("Receive {} bytes of data from {}:{}.", bytes_recved, other_end.address().to_string(), other_end.port());


                std::string data (_receive_data, bytes_recved);
                spdlog::info("Data bytes in hex: ");
                printHexBytes(_receive_data, bytes_recved);

                _io_context.post([data, &other_end, this]()mutable {
                    handle_data(other_end, std::span<char>(data.begin(), data.end()));
                });


                do_receive();

            });
}

void UdpClient::do_send() {
    if(!_running) return;
    _socket.async_send_to(
            boost::asio::buffer(_send_data_deque.front().second, _send_data_deque.front().second.size()),
            _send_data_deque.front().first,
            _socket_write_strand.wrap([this](const boost::system::error_code& error, std::size_t bytes_transferred){
                spdlog::info("Send {} bytes'data from {}:{} to {}:{}",
                             bytes_transferred,
                             _socket.local_endpoint().address().to_string(),
                             _socket.local_endpoint().port(),
                             _send_data_deque.front().first.address().to_string(),
                             _send_data_deque.front().first.port()
                             );
                printHexBytes((const char*)_send_data_deque.front().second.data(),_send_data_deque.front().second.size());
                send_data_done(error);
            })
    );
}

void UdpClient::queue_send_data(const boost::asio::ip::udp::endpoint& otherend, std::vector<unsigned char> &data) {
    _io_context.post(_socket_write_strand.wrap([this, otherend, data](){
        if(!_running) return;
        bool in_progress = !_send_data_deque.empty();
        _send_data_deque.emplace_back(otherend,data);

        if(!in_progress){
            do_send();
        }
    }));

}

void UdpClient::send_data_done(const boost::system::error_code &error) {
    if(!_running) return;
    if(!error){
        _send_data_deque.pop_front();
        if(!_send_data_deque.empty()) do_send();
    }
}


void UdpClient::Start() {
    _running = true;
    do_receive();
    for(int i = 0; i < thread_pool_size; i++){
        _thread_pool.emplace_back([&]{_io_context.run();});
    }


    while(_running){}

    wait_for_all_tasks();
}


void UdpClient::wait_for_all_tasks() {
    for(auto &i : _thread_pool){
        if(i.joinable()) i.join();
    }
}


