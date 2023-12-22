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

UdpClient::UdpClient(boost::asio::io_context& io_context, std::string_view ip, uint16_t port)
    : _io_context(io_context),_socket_write_strand(_io_context),
      _socket(_io_context,
              boost::asio::ip::udp::endpoint(
                      boost::asio::ip::address::from_string(ip.data()),
                      port))
{
}
UdpClient::UdpClient(boost::asio::io_context& io_context, uint16_t port)
    : _io_context(io_context),_socket_write_strand(_io_context),
      _socket(_io_context,
              boost::asio::ip::udp::endpoint (
                      boost::asio::ip::udp::v4(),
                      port
                      ))

{

}

UdpClient::UdpClient(boost::asio::io_context& io_context)
    :_io_context(io_context),_socket_write_strand(_io_context),
     _socket(_io_context,boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4::any(), 0))
{
}




UdpClient::~UdpClient() {
    if(_socket.is_open()){
        _socket.close();
    }
}

void UdpClient::cancel_cb(uint64_t message_id){
    spdlog::info("Message {} is acked, remove related data from the acking list.", message_id);
    auto it = _acking_list.find(message_id);
    if(it != _acking_list.end()){
        _acking_list.erase(it);
    }
};

void UdpClient::error_cb (uint64_t message_id){};

void UdpClient::timeout_cb (uint64_t message_id){
    spdlog::info("Message {} timeout, resend the message to {}:{}, and remove from the acking list.", message_id, get_endpoint(message_id).address().to_string(),
                 get_endpoint(message_id).port());

    auto& timer = get_timer(message_id);
    timer.expires_after(std::chrono::milliseconds(100));
    timer.async_wait([message_id, &timer, this](const boost::system::error_code& ec){
        timer_callback(ec, timer, message_id,
                       [this](uint64_t mid){cancel_cb(mid);},
                       [this](uint64_t mid){error_cb(mid);},
                       [this](uint64_t mid){timeout_cb(mid);}
        );
    });


    _io_context.post(_socket_write_strand.wrap([this, message_id](){
        bool in_progress = !_send_data_deque.empty();
        _send_data_deque.emplace_back(
                std::move(get_endpoint(message_id)),
                std::move(get_data(message_id)),
                message_id,
                kReliable);


        auto it = _acking_list.find(message_id);
        if(it != _acking_list.end()){
            _acking_list.erase(it);
        }
        spdlog::info("send data deque size: {}", _send_data_deque.size());

        if(!in_progress) do_send();
    }));
};

void UdpClient::do_receive() {
    if(!_running) return;
    boost::asio::ip::udp::endpoint  other_end;
    _socket.async_receive_from(
            boost::asio::buffer(_receive_data, max_length), other_end,

            //TODO: capture &other_end should produce bugs, but it doesnt
            [this, &other_end](const boost::system::error_code& ec, std::size_t bytes_recved) {
                if (ec) {
                    spdlog::error("Error receive: {}", ec.message());
                }
                spdlog::info("Receive {} bytes of data from {}:{}.", bytes_recved, other_end.address().to_string(), other_end.port());


                std::string data (_receive_data, bytes_recved);
                printHexBytes(_receive_data, bytes_recved);

                _io_context.post([data, other_end, this]()mutable {
                    uint64_t ack_message_id, new_message_id;
                    parse_message_id({data.begin(), data.end()}, &ack_message_id, &new_message_id);
                    cancel_retry_timer(ack_message_id);
                    auto response = handle_data(other_end, {data.begin() + 2 * sizeof(uint64_t), data.end()});
                    if(!_running) return;
                    queue_send_data(std::get<0>(response), std::get<1>(response), std::get<2>(response),
                                    new_message_id);
                });

                do_receive();
            });
}

void UdpClient::do_send() {
    if(!_running) return;
    _socket.async_send_to(
            boost::asio::buffer(std::get<1>(_send_data_deque.front()), std::get<1>(_send_data_deque.front()).size()),
            std::get<0>(_send_data_deque.front()),
            _socket_write_strand.wrap([this](const boost::system::error_code& error, std::size_t bytes_transferred){
                if(error){
                    spdlog::error("error sending message to {}:{} : {}", std::get<0>(_send_data_deque.front()).address().to_string(),
                                  std::get<0>(_send_data_deque.front()).port() ,error.message());
                    exit(-1);
                }

                MessageType message_type = std::get<3>(_send_data_deque.front());
                uint64_t new_message_id = std::get<2>(_send_data_deque.front());
                const auto& endpoint = std::get<0>(_send_data_deque.front());
                const auto& data = std::get<1>(_send_data_deque.front());

                spdlog::info("Send {} bytes'data ({}, new message id: {}) from {}:{} to {}:{}",
                             bytes_transferred,
                             message_type==kReliable?"reliable":"unreliable",
                             new_message_id,
                             _socket.local_endpoint().address().to_string(),
                             _socket.local_endpoint().port(),
                             endpoint.address().to_string(),
                             endpoint.port()
                );
                printHexBytes((const char *) data.data(),
                              data.size());
                send_data_done();
            })
    );
}

void UdpClient::queue_send_data(const boost::asio::ip::udp::endpoint &otherend, std::vector<unsigned char> data,
                                MessageType message_type, uint64_t message_id_to_ack) {
    _io_context.post(_socket_write_strand.wrap([this, _data = std::move(data), message_id_to_ack, otherend, message_type](){
        if(!_running) return;

        bool in_progress = !_send_data_deque.empty();
//        _send_data_deque.emplace_back(otherend,std::move(data));

        uint64_t new_message_id;
        // wrap new message with new mid
        auto packet = wrap_packet(_data, message_id_to_ack, &new_message_id);
        spdlog::info("Emplacing {} packet id: {}, acking pakcet {}", message_type == kReliable ? "Reliable" : "Unreliable", new_message_id, message_id_to_ack);
        _send_data_deque.emplace_back(otherend, std::move(packet), new_message_id,  message_type);
        spdlog::info("send data deque size: {}", _send_data_deque.size());

        if(!in_progress){
            do_send();
        }
    }));

}

void UdpClient::send_data_done() {
    if(!_running) return;
    auto message_type = std::get<3>(_send_data_deque.front());
    if(message_type == MessageType::kReliable){
        spdlog::info("Launching retry timer for message id: {}", std::get<2>(_send_data_deque.front()));
        // move the data to acking list, avoid copying
        launch_retry_timer(
                std::get<2>(_send_data_deque.front()),
                std::move(std::get<1>(_send_data_deque.front())),
                std::get<0>(_send_data_deque.front()),
                [this](uint64_t mid) { cancel_cb(mid); },
                [this](uint64_t mid) { error_cb(mid); },
                [this](uint64_t mid) { timeout_cb(mid); }
        );
    }
    _send_data_deque.pop_front();
    if(!_send_data_deque.empty()) do_send();
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

boost::asio::steady_timer &UdpClient::get_timer(uint64_t message_id) {
    auto it = _acking_list.find(message_id);
    assert(it != _acking_list.end());
    return std::get<0>(it->second);
}
std::vector<unsigned char> &UdpClient::get_data(uint64_t message_id) {
    auto it = _acking_list.find(message_id);
    assert(it != _acking_list.end());
    return std::get<1>(it->second);
}

boost::asio::ip::udp::endpoint &UdpClient::get_endpoint(uint64_t message_id) {
    auto it = _acking_list.find(message_id);
    assert(it != _acking_list.end());
    return std::get<2>(it->second);
}

void UdpClient::launch_retry_timer(uint64_t message_id,
                                   std::vector<unsigned char> data,
                                   const boost::asio::ip::udp::endpoint &endpoint,
                                   const std::function<void(uint64_t)> &cancel_cb_,
                                   const std::function<void(uint64_t)> &error_cb_,
                                   const std::function<void(uint64_t)> &timeout_cb_)
{
    auto it = _acking_list.find(message_id);
    if(it != _acking_list.end()){
        spdlog::warn("Retry timer of block {}, slice {} has already been launched, repeated request.");
        return;
    }
    boost::asio::steady_timer timer {_io_context};
    timer.expires_after(std::chrono::milliseconds(500));
    _acking_list.emplace(message_id, std::make_tuple(std::move(timer), std::move(data), endpoint));
    get_timer(message_id).async_wait([this, message_id, cancel_cb_, error_cb_, timeout_cb_](const boost::system::error_code& ec){
        timer_callback(ec, get_timer(message_id), message_id, cancel_cb_, error_cb_, timeout_cb_);
    });

}

void UdpClient::cancel_retry_timer(uint64_t message_id) {
    auto it = _acking_list.find(message_id);
    if(it != _acking_list.end()){
        spdlog::info("Cancel timer for reliable message {}", message_id);
        std::get<0>(it->second).cancel();
    }
}

void UdpClient::timer_callback(const boost::system::error_code &ec, boost::asio::steady_timer &timer, uint64_t message_id,
                               const std::function<void(uint64_t)> &cancel_cb_, const std::function<void(uint64_t)> &error_cb_,
                               const std::function<void(uint64_t)> &timeout_cb_) {
    if(ec == boost::asio::error::operation_aborted){
        spdlog::warn("Retry timer for message {} canceled.", message_id);
        cancel_cb_(message_id);
    }
    else if(ec){
        spdlog::error("Retry timer for message {} error: {}.", message_id, ec.message());
        error_cb_(message_id);
        exit(-1);
    }
    else{
        spdlog::warn("Timeout for message {} .", message_id);
        timeout_cb_(message_id);
    }
}

std::vector<unsigned char>
UdpClient::wrap_packet(const std::vector<unsigned char> &message, uint64_t message_id_to_ack, uint64_t *new_message_id) {
    std::vector<unsigned char> packet;
    *new_message_id = ++_message_id;
    std::vector<uint64_t> mid = {message_id_to_ack, *new_message_id};
    auto &mid_ = reinterpret_cast<std::vector<unsigned char>&>(mid);
    packet.insert(packet.end(), mid_.begin(), mid_.end());
    packet.insert(packet.end(), message.begin(), message.end());

    return std::move(packet);
}

void UdpClient::parse_message_id(std::span<char> data, uint64_t *ack_message_id, uint64_t *new_message_id) {
    assert(data.size() >= 2*sizeof(uint64_t));
    *ack_message_id = *reinterpret_cast<uint64_t*>(&*data.begin());
    *new_message_id = *reinterpret_cast<uint64_t*>(&*data.begin() + sizeof(uint64_t));
}

void UdpClient::clear_acking_list() {
    auto iter = _acking_list.begin();
    while(iter != _acking_list.end()){
        auto & timer = std::get<0>((*iter).second);
        timer.cancel();
        iter = _acking_list.erase(iter);
    }
}


