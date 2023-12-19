//
// Created by zaiyichen on 2023/11/29.
//

#ifndef IMGUIOPENGL_SENDER_H
#define IMGUIOPENGL_SENDER_H

#include <core/UdpClient.h>

class Sender : public UdpClient{

public:
    Sender(uint16_t port, std::string_view filename);
    ~Sender() override;

protected:
    void handle_data(boost::asio::ip::udp::endpoint endpoint, std::span<char> data) override;

private:

    void load_block(int id);

    std::shared_ptr<std::ifstream> _ifs;
    uint64_t _file_size;
    uint64_t _block_count;
    std::string _file_name;
    std::unique_ptr<BufferBlock> _current_buffer_block;
    int _current_buffer_block_id{ -1};

    boost::asio::steady_timer _close_wait_timer;

    bool _transferring;


    boost::asio::ip::udp::endpoint _receiver_endpoint;
};


#endif //IMGUIOPENGL_SENDER_H
