//
// Created by zaiyichen on 2023/11/29.
//

#include <core/Client.h>

Client::Client(std::string_view ip, uint16_t port, int block_id)
    : _io_context(), _socket(_io_context, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 0)),
      _socket_write_strand(_io_context), _current_block_id(block_id), _current_block_slice_id(0),
      _server_endpoint(boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(ip.data()), port))
      {
    do_send();
    for(int i = 0; i < thread_pool_count; i++){
        _thread_pool.emplace_back([&]{_io_context.run();});
    }
    for(auto & i : _thread_pool){
        i.join();
    }
}

void Client::do_receive() {

}

void Client::do_send() {
    _socket.async_send_to(boost::asio::buffer(_send_data_deque.front(), _send_data_deque.front().size()),
                          _server_endpoint,
                          _socket_write_strand.wrap([this](const boost::system::error_code& error, std::size_t bytes_transferred) {
                              send_data_done(error);
                          }));
}

void Client::handle_data() {

}

void Client::queue_send_data() {

}

void Client::send_data_done(const boost::system::error_code &error) {
    if(!error){
        _send_data_deque.pop_front();
        if(!_send_data_deque.empty()) do_send();
    }
}
