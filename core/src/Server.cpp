//
// Created by zaiyichen on 2023/11/29.
//

#include <iostream>
#include <core/Server.h>
#include <spdlog/spdlog.h>

Server::Server(uint16_t port)
        : _io_context(), _socket(_io_context, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(),port)),
          _write_strand(_io_context)

{
    do_receive();
    for(int i = 0; i < thread_pool_size; i++){
        _thread_pool.emplace_back([&]{_io_context.run();});
    }
    for(int i = 0; i < thread_pool_size; i++){
        _thread_pool[i].join();
    }
}

void Server::do_receive() {
    _socket.async_receive_from(
            boost::asio::buffer(_receive_data, max_length), _sender_endpoint,
            [this](boost::system::error_code ec, std::size_t bytes_recvd) {
                if (ec) return;
                auto data_span = handle_data(bytes_recvd);

                _io_context.post(_write_strand.wrap([this, data_span]{
                    queue_send_data(data_span);
                }));
                do_receive();
            });
}

void Server::do_send() {
    _socket.async_send_to(
    boost::asio::buffer(_send_data_deque.front(), _send_data_deque.front().size()),
//    _send_data_deque.front(),
        _sender_endpoint,
        _write_strand.wrap([this](const boost::system::error_code& error, std::size_t bytes_transferred){
            send_data_done(error);
        })
    );


}

std::string temp_test;

std::span<char> Server::handle_data(std::size_t length) {
    auto data = std::string_view (_receive_data, _receive_data + length);
    spdlog::info("Receive {} bytes of data: {}.", length, data );

    // parse data
    // retrieve file block slice from the file manager.

    temp_test += "1";
    return {temp_test.begin(), temp_test.end()};

}

void Server::queue_send_data(std::span<char> data) {
    bool in_progress = !_send_data_deque.empty();
    _send_data_deque.push_back(data);

    if(!in_progress){
        do_send();
    }
}

void Server::send_data_done(const boost::system::error_code &error) {
    if(!error){
        _send_data_deque.pop_front();
        if(!_send_data_deque.empty()) do_send();
    }
}

int main(int argc, char** argv){
    try
    {

        Server s(std::atoi(argv[1]));

    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
