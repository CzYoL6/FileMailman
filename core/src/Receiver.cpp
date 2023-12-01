//
// Created by zaiyichen on 2023/11/29.
//

#include <core/Receiver.h>
#include <core/Message.h>
#include <spdlog/spdlog.h>

Receiver::Receiver(std::string_view ip, uint16_t port, int block_id)
    : UdpClient(ip, port),
      _current_block_id(block_id),
      _current_block_slice_id(0)
{

    _current_buffer_block = std::make_unique<BufferBlock>(slice_size, block_size, _io_context);

}


Receiver::~Receiver() {
    if(_ofs->is_open()){
        _ofs->close();
    }
}

std::variant<std::span<char>, std::vector<unsigned char>> Receiver::handle_data(std::span<char> data) {
    auto [senderMsgHeader, load] = Message::GetSenderHeader(data);
    switch(senderMsgHeader) {
        case Message::SenderMsgHeader::kBeginTransferAck_FileMeta: {
            int file_name_size = *(reinterpret_cast<int*>(load.data()));
            assert(file_name_size > 0);
            std::string file_name = {load.data() + sizeof(file_name_size), load.data() + sizeof(file_name_size) + file_name_size};
            assert(!file_name.empty());
            int file_size = *(reinterpret_cast<int*>(load.data() + sizeof(file_name_size) + file_name_size));
            assert(file_size > 0);

            spdlog::info(" Sender Begin Transfer Ack, file name: {}, file size: {}", file_name, file_size);
            _file_size = file_size;
            _file_name = file_name;
            _block_count = (_file_size % block_size == 0) ? _file_size / block_size : _file_size / block_size + 1;
            break;
        }
        default: {
            spdlog::error("Message parsing failed");
            exit(-1);
            break;
        }
    }
}

void Receiver::save_block(int id) {

}
