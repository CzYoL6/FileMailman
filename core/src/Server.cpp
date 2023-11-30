//
// Created by zaiyichen on 2023/11/29.
//

#include <iostream>
#include <core/Server.h>
#include <spdlog/spdlog.h>
#include <core/Message.h>
#include <fstream>
#include <filesystem>

Server::Server(uint16_t port, std::string_view filename)
        : _io_context(),
          _socket(_io_context, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(),port)),
          _socket_write_strand(_io_context),
          _file_name(filename),
          _ifs(std::make_unique<std::ifstream >(filename.data())),
          _close_wait_timer(_io_context)
{

    _file_size = std::filesystem::file_size(filename);
    _current_buffer_block = std::make_unique<BufferBlock>(slice_size, block_size, _io_context);
    _block_count = (_file_size % block_size == 0) ? _file_size / block_size : _file_size / block_size + 1;

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
            boost::asio::buffer(_receive_data, max_length), _client_endpoint,
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

void Server::do_send() {
    _socket.async_send_to(
            std::holds_alternative<std::span<char>>(_send_data_deque.front()) ?
                boost::asio::buffer(std::get<std::span<char>>(_send_data_deque.front()), std::get<std::span<char>>(_send_data_deque.front()).size()) :
                boost::asio::buffer(std::get<std::vector<unsigned char>>(_send_data_deque.front()), std::get<std::vector<unsigned char>>(_send_data_deque.front()).size())
                ,
            _client_endpoint,
            _socket_write_strand.wrap([this](const boost::system::error_code& error, std::size_t bytes_transferred){
                      send_data_done(error);
                })
    );


}


std::variant<std::span<char>, std::vector<unsigned char>> Server::handle_data(std::span<char> data) {

    auto [clientMsgHeader, load] = Message::GetClientHeader(data );
    switch(clientMsgHeader){
        case Message::ClientMsgHeader::kBeginTransfer: {
            assert(load.empty());
            spdlog::info("Client Send Begin Transfer");

            std::vector<unsigned char> message = {
                    (uint8_t)Message::ServerMsgHeader::kBeginTransferAck_FileMeta,   //header
                    0x08, 0x00, 0x00,  0x00,   //8
                    0x74, 0x65, 0x73, 0x74, 0x2e, 0x70, 0x6e, 0x67,   //test.png
                    0x61, 0xbf, 0x91};
            return std::move(message);
            break;
        }
        case Message::ClientMsgHeader::kBeginBlock: {
            assert(load.size() == sizeof(int));
            int block_id = *(reinterpret_cast<int*>(load.data()));
            spdlog::info("Client Send Begin Block {}", block_id);

            load_block(block_id);

            std::vector<unsigned char> message = {
                    (uint8_t)Message::ServerMsgHeader::kBeginBlockAck
            };
            return std::move(message);
            break;
        }
        case Message::ClientMsgHeader::kRequireSlice: {
            assert(load.size() == sizeof(int));

            int slice_id = *(reinterpret_cast<int *>(load.data()));
            assert(slice_id < _current_buffer_block->slice_count());

            spdlog::info("Client Send Require Slice {}", slice_id);

            std::span<char> slice_data = _current_buffer_block->GetBlockSlice(slice_id)->data_span() ;

            return slice_data;

            break;
        }
        case Message::ClientMsgHeader::kEndTransfer: {
            assert(load.empty());
            spdlog::info("Client Send End Transfer");

            std::vector<unsigned char> message = {
                    (uint8_t)Message::ServerMsgHeader::kEndTransferAck
            };

            _close_wait_timer.cancel();
            _close_wait_timer.expires_after(std::chrono::seconds (5));
            _close_wait_timer.async_wait([&](const  boost::system::error_code& ec){
                if(ec == boost::asio::error::operation_aborted){
                    spdlog::warn("Receive redundant end transfer message, restart timer.");
                }
                else if(ec){
                    spdlog::error("Close wait timer error.");
                    exit(-1);
                }
                else{
                    spdlog::warn("Ready to end program.");
                    _io_context.stop();
                }
            });

            return std::move(message);

            break;
        }
        default: {
            spdlog::error("Message parsing failed");
            exit(-1);
            break;
        }
    }

}

void Server::queue_send_data(const std::variant<std::span<char>, std::vector<unsigned char>> &data) {
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

void Server::load_block(int id) {
    if(_current_buffer_block_id != id){
        _current_buffer_block->ReadFromFile(*_ifs, id * block_size,
                                            std::min((uint64_t)block_size, _file_size - id * block_size));
    }
}

Server::~Server() {
    for(auto &i : _thread_pool){
        if(i.joinable()) i.join();
    }
    if(_ifs->is_open()){
        _ifs->close();
    }
    if(_socket.is_open()){
        _socket.close();
    }
}

int main(int argc, char** argv){
    try
    {

        Server s(std::atoi(argv[1]), "test.png");

    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
