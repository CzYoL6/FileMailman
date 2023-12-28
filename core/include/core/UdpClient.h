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

enum MessageType: uint8_t {
    kReliable = 0,
    kUnreliable
};
class UdpClient {
protected:
    struct HandleDataRetItem{
        boost::asio::ip::udp::endpoint endpoint;
        std::vector<unsigned char> data;
        MessageType message_type;
    };
    using HandleDataRetValue = std::vector<HandleDataRetItem>;

    struct SendDequeItem{
        boost::asio::ip::udp::endpoint endpoint;
        std::vector<unsigned char> data;
        uint64_t new_message_id;    // new mid
        MessageType message_type;
    };
    struct AckingListItem{
        boost::asio::steady_timer timer;
        std::vector<unsigned char> data;
        boost::asio::ip::udp::endpoint endpoint;
    };

public:
    enum { max_length = 2048, thread_pool_size = 8, block_size = 1024 * 8, slice_size = 1024};
    explicit UdpClient(boost::asio::io_context& io_context, std::string_view ip, uint16_t port);
    explicit UdpClient(boost::asio::io_context& io_context, uint16_t port);
    explicit UdpClient(boost::asio::io_context& io_context);
    virtual ~UdpClient();
    void Start();

protected:
    void wait_for_all_tasks();
    void queue_send_data(const boost::asio::ip::udp::endpoint &otherend, std::vector<unsigned char> data,
                         MessageType message_type, uint64_t message_id_to_ack);
    void do_receive();
    void do_send();
    virtual HandleDataRetValue
    handle_data(boost::asio::ip::udp::endpoint endpoint, std::span<char> data) =0;
    void send_data_done();
    void clear_acking_list();

private:
    AckingListItem&  get_acking_item(uint64_t message_id);   // no internal multithread protection, do wrap this in strand
    void remove_acking_item(uint64_t message_id);

    static void parse_message_id(std::span<char> data, uint64_t *ack_message_id, uint64_t *new_message_id);

    void launch_retry_timer(uint64_t message_id, std::vector<unsigned char> data,
                            const boost::asio::ip::udp::endpoint &endpoint,
                            const std::function<void(uint64_t)> &cancel_cb_,
                            const std::function<void(uint64_t)> &error_cb_,
                            const std::function<void(uint64_t)> &timeout_cb_);
    void cancel_retry_timer(uint64_t message_id);
    static void timer_callback(const boost::system::error_code &ec, boost::asio::steady_timer &timer, uint64_t message_id,
                               const std::function<void(uint64_t)> &cancel_cb_, const std::function<void(uint64_t)> &error_cb_,
                               const std::function<void(uint64_t)> &timeout_cb_);
    std::vector<unsigned char>
    wrap_packet(const std::vector<unsigned char> &message, uint64_t message_id_to_ack, uint64_t *new_message_id);
    void cancel_cb(uint64_t message_id);
    void error_cb(uint64_t message_id);
    void timeout_cb(uint64_t message_id);

protected:
    boost::asio::io_context& _io_context;
    boost::asio::ip::udp::socket _socket;
    char _receive_data[max_length]{};
    std::vector<std::thread> _thread_pool;

    boost::asio::io_context::strand _send_data_deque_strand;
    std::deque<SendDequeItem> _send_data_deque;
    bool _running{false};

    uint64_t _message_id{0};

    boost::asio::io_context::strand _acking_list_strand;
    std::unordered_map<uint64_t ,AckingListItem> _acking_list;
};


#endif //IMGUIOPENGL_UDPCLIENT_H
