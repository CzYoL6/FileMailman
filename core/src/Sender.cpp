//
// Created by zaiyichen on 2023/11/29.
//

#include <iostream>
#include <core/Sender.h>
#include <spdlog/spdlog.h>
#include <core/Message.h>
#include <fstream>
#include <filesystem>

Sender::Sender(boost::asio::io_context &io_context, uint16_t port, std::string_view filename) :
        UdpClient(io_context,port),
          _file_name(filename),
          _ifs(std::make_shared<std::ifstream >(filename.data(), std::ios::binary)),
          _close_wait_timer(io_context),
          _transferring(false),
          _file_size(std::filesystem::file_size(filename)),
          _current_buffer_block(std::make_unique<BufferBlock>(0, 0, 0)),
          _block_count((_file_size % block_size == 0) ? _file_size / block_size : _file_size / block_size + 1)

{
    spdlog::warn("server launched on port {}, file name : {}, file name size: {} bytes, file size: {} bytes, block count: {}",
                 port,
                 _file_name,
                 _file_name.size(),
                 _file_size,
                 _block_count
                 );
}


Sender::HandleDataRetValue
Sender::handle_data(boost::asio::ip::udp::endpoint endpoint, std::span<char> data) {
    if(!_running) return{};
    spdlog::info("handling data...");
    auto [clientMsgHeader, load] = Message::GetReceiverHeader(data);
    switch(clientMsgHeader){
        case Message::ReceiverMsgHeader::kBeginTransfer: {
            spdlog::info("Receiver Send Begin Transfer");
            assert(load.empty());

            if(_transferring){
                break;
            }
            _transferring = true;

            _receiver_endpoint = endpoint;

            return generate_begin_transfer_ack(_file_name, (int) _file_size);
            break;
        }
        case Message::ReceiverMsgHeader::kBeginBlock: {
            assert(load.size() == sizeof(int));
            int block_id = *(reinterpret_cast<int*>(load.data()));
            spdlog::info("Receiver Send Begin Block {}", block_id);

            if(block_id != _current_buffer_block_id + 1){
                spdlog::warn("Transferring of block {} not correct!", block_id);
                break;
            }
            _current_buffer_block_id = block_id;
            int real_block_size = std::min((int)block_size, (int)(_file_size - _current_buffer_block_id * block_size));
            _current_buffer_block = std::make_unique<BufferBlock>(_current_buffer_block_id, slice_size, real_block_size);
            load_block(block_id);

            return generate_begin_block_ack(block_id);
            break;
        }
        case Message::ReceiverMsgHeader::kRequireSlice: {
            assert(load.size() == 2 * sizeof(int));

            int block_id = *(reinterpret_cast<int *>(load.data()));
            assert(block_id < _block_count);

            int slice_id = *(reinterpret_cast<int *>(load.data() + 4));
            assert(slice_id < _current_buffer_block->slice_count());

            spdlog::info("Receiver Send Require Slice {} (max {}) of Block {} (max {})",
                         slice_id,
                         _current_buffer_block->slice_count() - 1,
                         block_id,
                         _block_count - 1
                         );

            if(block_id != _current_buffer_block_id){
                spdlog::warn("Not current block id, redundant packet detected!");
                break;
            }

            std::span<char> slice_data = _current_buffer_block->GetBlockSlice(slice_id)->data_span() ;

            return generate_slice_data(block_id, slice_id, slice_data);
            break;
        }
        case Message::ReceiverMsgHeader::kEndTransfer: {
            assert(load.empty());
            spdlog::info("Receiver Send End Transfer");


            _close_wait_timer.cancel();
            _close_wait_timer.expires_after(std::chrono::seconds (5));
            _close_wait_timer.async_wait([this](const  boost::system::error_code& ec){
                if(ec == boost::asio::error::operation_aborted){
                    spdlog::warn("Receive redundant end transfer message, restart timer.");
                }
                else if(ec){
                    spdlog::error("Close wait timer error.");
                    exit(-1);
                }
                else{
                    spdlog::warn("Ready to end program.");
                    _running = false;
                    _ifs->close();
                    _io_context.stop();
                }
            });


            return generate_end_transfer_ack();
            break;
        }
        default: {
            spdlog::error("Message parsing failed");
            exit(-1);
            break;
        }
    }

    return {};
}

void Sender::load_block(int id) {
    if(!_running) return;
    int bytes_size = std::min((uint64_t)block_size, _file_size - id * block_size);
    _current_buffer_block->ReadFromFile(*_ifs, id * block_size, bytes_size);
    spdlog::info("Load {} bytes' data to block {} (max {})", bytes_size, id, _block_count - 1);
}

Sender::~Sender() {
    if(_ifs->is_open()){
        _ifs->close();
    }

}

Sender::HandleDataRetItem Sender::generate_begin_transfer_ack(const std::string &file_name, int file_size) { spdlog::info("Send begin transfer ack");
    std::vector<unsigned char> message = {
            (uint8_t)Message::SenderMsgHeader::kBeginTransferAck_FileMeta   //header
    };
    std::vector<int> file_name_size = {
            static_cast<int>(file_name.size())
    };

    auto& file_name_size_ = reinterpret_cast<std::vector<unsigned char>&>(file_name_size);
    std::vector<int> sz = {file_size};
    auto& file_size_ = reinterpret_cast<std::vector<unsigned char>&>(sz);

    message.insert(message.end(), file_name_size_.begin(), file_name_size_.end());
    for(const unsigned char c : file_name){
        message.push_back(c);
    }
    message.insert(message.end(), file_size_.begin(), file_size_.end());

    return {_receiver_endpoint, std::move(message),kUnreliable};
}

Sender::HandleDataRetItem Sender::generate_begin_block_ack(int block_id) {
    spdlog::info("Send begin block ack, block id: {}", block_id);
    std::vector<unsigned char> message = {
            (uint8_t)Message::SenderMsgHeader::kBeginBlockAck,
    };
    std::vector<int> id = {block_id};
    auto& id_ = reinterpret_cast<std::vector<unsigned char>&>(id);
    message.insert(message.end(), id_.begin(), id_.end());

    return {_receiver_endpoint, std::move(message),kUnreliable};
}

Sender::HandleDataRetItem Sender::generate_slice_data(int block_id, int slice_id, std::span<char> slice_data) {
    spdlog::info("Send slice data, block {}, slice {}", block_id, slice_id);
    std::vector<unsigned char> message = {
            (uint8_t)Message::SenderMsgHeader::kSliceData
    };
    std::vector<int> ids = {block_id, slice_id};
    auto& ids_ = reinterpret_cast<std::vector<unsigned char>&>(ids);
    message.insert(message.end(), ids_.begin(), ids_.end());
    message.insert(message.end(), slice_data.begin(), slice_data.end());

    return {_receiver_endpoint, std::move(message),kUnreliable};
}

Sender::HandleDataRetItem Sender::generate_end_transfer_ack() {
    spdlog::info("Send end transfer ack");
    std::vector<unsigned char> message = {
            (uint8_t)Message::SenderMsgHeader::kEndTransferAck
    };

    return {_receiver_endpoint, std::move(message), kUnreliable};
}

