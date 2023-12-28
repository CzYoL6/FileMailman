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
    : _io_context(io_context), _send_data_deque_strand(_io_context), _acking_list_strand(_io_context),
      _socket(_io_context,
              boost::asio::ip::udp::endpoint(
                      boost::asio::ip::address::from_string(ip.data()),
                      port))
{
}
UdpClient::UdpClient(boost::asio::io_context& io_context, uint16_t port)
    : _io_context(io_context), _send_data_deque_strand(_io_context), _acking_list_strand(_io_context),
      _socket(_io_context,
              boost::asio::ip::udp::endpoint (
                      boost::asio::ip::udp::v4(),
                      port
                      ))

{

}

UdpClient::UdpClient(boost::asio::io_context& io_context)
    : _io_context(io_context), _send_data_deque_strand(_io_context), _acking_list_strand(_io_context),
      _socket(_io_context,boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4::any(), 0))
{
}




UdpClient::~UdpClient() {
    if(_socket.is_open()){
        _socket.close();
    }
}

void UdpClient::cancel_cb(uint64_t message_id){};

void UdpClient::error_cb (uint64_t message_id){};

void UdpClient::timeout_cb (uint64_t message_id){
    _io_context.post(_acking_list_strand.wrap([this, message_id](){
        auto& acking_item = get_acking_item(message_id);
        spdlog::info("Message {} timeout, resend the message to {}:{}, and remove from the acking list.",
                     message_id,
                     acking_item.endpoint.address().to_string(),
                     acking_item.endpoint.port()
        );

        auto& timer = get_acking_item(message_id).timer;
        timer.expires_after(std::chrono::milliseconds(100));
        timer.async_wait([message_id, &timer, this](const boost::system::error_code& ec){
            timer_callback(ec, timer, message_id,
                           [this](uint64_t mid){cancel_cb(mid);},
                           [this](uint64_t mid){error_cb(mid);},
                           [this](uint64_t mid){timeout_cb(mid);}
            );
        });


        _io_context.post(_send_data_deque_strand.wrap([this, message_id](){
            bool in_progress = !_send_data_deque.empty();
            auto& acking_item = get_acking_item(message_id);
            _send_data_deque.emplace_back(
                    std::move(acking_item.endpoint),
                    std::move(acking_item.data),
                    message_id,
                    kReliable);

            remove_acking_item(message_id);

            if(!in_progress) do_send();
        }));
    }));

};

void UdpClient::do_receive() {
    if(!_running) return;
    std::shared_ptr<boost::asio::ip::udp::endpoint>  other_end = std::make_shared<boost::asio::ip::udp::endpoint>();
    _socket.async_receive_from(
            boost::asio::buffer(_receive_data, max_length), *other_end,

            [this, other_end](const boost::system::error_code& ec, std::size_t bytes_recved) {
                if (ec) {
                    spdlog::error("Error receive: {}", ec.message());
                }
                spdlog::info("Receive {} bytes of data from {}:{}.", bytes_recved, other_end->address().to_string(), other_end->port());


                std::string data (_receive_data, bytes_recved);
                printHexBytes(_receive_data, bytes_recved);

                _io_context.post([data_ = std::move(data), other_end, this]()mutable {
                    uint64_t ack_message_id, new_message_id;
                    parse_message_id({data_.begin(), data_.end()}, &ack_message_id, &new_message_id);
                    cancel_retry_timer(ack_message_id);
                    auto responses = handle_data(*other_end, {data_.begin() + 2 * sizeof(uint64_t), data_.end()});
                    if(responses.empty()) return;
                    if(!_running) return;
                    for(const auto& response:responses) {
                        queue_send_data(response.endpoint, response.data, response.message_type, new_message_id);
                    }
                });

                do_receive();
            });
}

void UdpClient::do_send() {
    if(!_running) return;
    _socket.async_send_to(
            boost::asio::buffer(_send_data_deque.front().data, _send_data_deque.front().data.size()),
            _send_data_deque.front().endpoint,
            _send_data_deque_strand.wrap([this](const boost::system::error_code& error, std::size_t bytes_transferred){
                if(error){
                    spdlog::error("error sending message to {}:{} : {}", _send_data_deque.front().endpoint.address().to_string(),
                                  _send_data_deque.front().endpoint.port() ,error.message());
                    exit(-1);
                }

                MessageType message_type = _send_data_deque.front().message_type;
                uint64_t new_message_id = _send_data_deque.front().new_message_id;
                const auto& endpoint = _send_data_deque.front().endpoint;
                const auto& data = _send_data_deque.front().data;

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
    _io_context.post(_send_data_deque_strand.wrap([this, _data = std::move(data), message_id_to_ack, otherend, message_type](){
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
    auto message_type = _send_data_deque.front().message_type;
    if(message_type == MessageType::kReliable){
        spdlog::info("Launching retry timer for message id: {}", _send_data_deque.front().new_message_id);
        // move the data to acking list to avoid copying
        launch_retry_timer(
                _send_data_deque.front().new_message_id,
                std::move(_send_data_deque.front().data),
                _send_data_deque.front().endpoint,
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

void UdpClient::launch_retry_timer(uint64_t message_id,
                                   std::vector<unsigned char> data,
                                   const boost::asio::ip::udp::endpoint &endpoint,
                                   const std::function<void(uint64_t)> &cancel_cb_,
                                   const std::function<void(uint64_t)> &error_cb_,
                                   const std::function<void(uint64_t)> &timeout_cb_)
{
    _io_context.post(_acking_list_strand.wrap([this, data_ = std::move(data), message_id, endpoint, cancel_cb_, error_cb_, timeout_cb_]() mutable {
        auto it = _acking_list.find(message_id);
        if(it != _acking_list.end()){
            spdlog::warn("Retry timer of block {}, slice {} has already been launched, repeated request.");
            return;
        }
        boost::asio::steady_timer timer {_io_context};
        timer.expires_after(std::chrono::milliseconds(500));
        auto [new_item, _] = _acking_list.emplace(message_id, AckingListItem{std::move(timer), std::move(data_), endpoint});
        new_item->second.timer.async_wait([this, new_item, message_id, cancel_cb_, error_cb_, timeout_cb_](const boost::system::error_code& ec){
            timer_callback(ec, new_item->second.timer, message_id, cancel_cb_, error_cb_, timeout_cb_);
        });
    }));


}

void UdpClient::cancel_retry_timer(uint64_t message_id) {
    _io_context.post(_acking_list_strand.wrap([this, message_id](){
        auto it = _acking_list.find(message_id);
        if(it != _acking_list.end()){
            spdlog::info("Cancel timer for reliable message {}", message_id);
            it->second.timer.cancel();
            _acking_list.erase(it);
        }
        else{
            spdlog::info("Cannot find timer for message {}, it is unreliable", message_id);
        }
    }));

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

std::vector<unsigned char> UdpClient::wrap_packet(const std::vector<unsigned char> &message, uint64_t message_id_to_ack, uint64_t *new_message_id) {
    std::vector<unsigned char> packet;
    *new_message_id = ++_message_id;
    std::vector<uint64_t> mid = {message_id_to_ack, *new_message_id};
    auto &mid_ = reinterpret_cast<std::vector<unsigned char>&>(mid);
    packet.insert(packet.end(), mid_.begin(), mid_.end());
    packet.insert(packet.end(), message.begin(), message.end());

    return packet;   // NRVO
}

void UdpClient::parse_message_id(std::span<char> data, uint64_t *ack_message_id, uint64_t *new_message_id) {
    assert(data.size() >= 2*sizeof(uint64_t));
    *ack_message_id = *reinterpret_cast<uint64_t*>(&*data.begin());
    *new_message_id = *reinterpret_cast<uint64_t*>(&*data.begin() + sizeof(uint64_t));
}

void UdpClient::clear_acking_list() {
    _io_context.post(_acking_list_strand.wrap([this](){
        spdlog::warn("Clearing acking list");
        auto iter = _acking_list.begin();
        while(iter != _acking_list.end()){
            auto & timer = (*iter).second.timer;
            timer.cancel();
            iter = _acking_list.erase(iter);
        }
    }));

}

UdpClient::AckingListItem &UdpClient::get_acking_item(uint64_t message_id) {
    auto it = _acking_list.find(message_id);
    if(it!=_acking_list.end()){
        return it->second;
    }
    else{
        spdlog::error("cannot find acking item with message id {}", message_id);
        exit(-1);
    }
}

void UdpClient::remove_acking_item(uint64_t message_id) {
    _io_context.post(_acking_list_strand.wrap([this, message_id](){
        spdlog::info("Remove acking item {} from list", message_id);
        auto it = _acking_list.find(message_id);
        if(it != _acking_list.end()){
            _acking_list.erase(it);
        }
        else{
            spdlog::error("Error removing acking item {}", message_id);
            exit(-1);
        }
    }));
}


