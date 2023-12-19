//
// Created by zaiyichen on 2023/11/29.
//

#include <core/Receiver.h>
#include <core/Message.h>
#include <spdlog/spdlog.h>
#include <iostream>
#include <fstream>

Receiver::Receiver(boost::asio::io_context &io_context, std::string_view ip, uint16_t port)
    : UdpClient(io_context),
    _sender_endpoint(),
    _file_size(0),
    _block_count(0),
    _current_buffer_block(std::make_unique<BufferBlock>(0, 0 , 0, _io_context))
{
    spdlog::warn("client launched. ");

    _sender_endpoint = boost::asio::ip::udp::endpoint(
            boost::asio::ip::address::from_string(ip.data()), port);

    _socket.connect(_sender_endpoint);

    // send begin transfer message
    send_begin_transfer();
}

Receiver::~Receiver() {
    if(_ofs && _ofs->is_open()){
        _ofs->close();
    }
}

void Receiver::handle_data(boost::asio::ip::udp::endpoint endpoint, std::span<char> data) {
    if(!_running) return;
    spdlog::info("handling data...");
    auto [senderMsgHeader, load] = Message::GetSenderHeader(data);
    switch(senderMsgHeader) {
        case Message::SenderMsgHeader::kBeginTransferAck_FileMeta: {
            int file_name_size = *(reinterpret_cast<int*>(load.data()));
            assert(file_name_size > 0);
            std::string file_name = {load.data() + sizeof(file_name_size), load.data() + sizeof(file_name_size) + file_name_size};
            assert(!file_name.empty());
            int file_size = *(reinterpret_cast<int*>(load.data() + sizeof(file_name_size) + file_name_size));
            assert(file_size > 0);

            spdlog::info(" Message Begin Transfer is Acked, file name: {}, file size: {}", file_name, file_size);
            _file_size = file_size;
            _file_name = file_name;
            _block_count = (_file_size % block_size == 0) ? _file_size / block_size : _file_size / block_size + 1;
            _ofs = std::make_shared<std::ofstream >("copy_" + _file_name, std::ios::binary);


            // start to ask for the first block (id 0)
            _current_block_id = 0;
            int real_block_size = std::min((int)block_size, (int)(_file_size - _current_block_id * block_size));
            _current_buffer_block = std::make_unique<BufferBlock>(_current_block_id, slice_size, real_block_size, _io_context);
            send_begin_block(0);
            break;
        }
        case Message::SenderMsgHeader::kBeginBlockAck:{
            int block_id = *(reinterpret_cast<int*>(load.data()));
            spdlog::info("Message Begin Block is acked, block id: {}", block_id);

            if(block_id != _current_block_id){
                break;
            }

            _current_block_slice_id = 0;
            // start to ask for the first slice in the block
            send_require_slice(_current_block_id, 0);
            break;
        }
        case Message::SenderMsgHeader::kSliceData: {
            int block_id = *(reinterpret_cast<int *>(load.data()));
            assert(block_id < _block_count);

            int slice_id = *(reinterpret_cast<int *>(load.data() + 4));
            assert(slice_id < _current_buffer_block->slice_count());

            spdlog::info("Receive slice data of slice {} (max {}), block {} (max {})",
                         slice_id,
                         _current_buffer_block->slice_count() - 1,
                         block_id,
                         _block_count - 1
                         );

            if(block_id != _current_block_id){
                spdlog::warn("Not current block id, redundant packet detected!");
                break;
            }

            if(slice_id != _current_block_slice_id){
                spdlog::warn("Not current slice id, redundant packet detected!");
                break;
            }

            std::span<char> slice_data(load.begin() + 8, load.end());
            _current_buffer_block->GetBlockSlice(slice_id)->WriteIn(slice_data.data(), slice_data.size());

            // require the next slice
            _current_block_slice_id++;
            if(_current_block_slice_id == _current_buffer_block->slice_count()){
                spdlog::warn("Transmission of block {} is finished, saving data.", _current_block_id);
                // save the block
                save_block(_current_block_id);

                // require next block
                _current_block_id++;
                if(_current_block_id == _block_count){
                    send_end_transfer();
                    break;
                }


                send_begin_block(_current_block_id);
                break;
            }


            send_require_slice(_current_block_id, _current_block_slice_id);

            break;

        }
        case Message::SenderMsgHeader::kEndTransferAck:{
            spdlog::warn("Message End Transfer is acked, ready to end program");
            _running = false;
            _ofs->close();
            _io_context.stop();
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
    if(!_running) return;
    int bytes_size = std::min((uint64_t)block_size, _file_size - id * block_size);
    _current_buffer_block->WriteToFile(*_ofs, id * block_size, bytes_size);
    spdlog::info("Save {} bytes' data of block {} (max {}) ", bytes_size, id, _block_count - 1);
}

void Receiver::send_begin_transfer() {
    spdlog::info("Send Begin Transfer message.");
    std::vector<unsigned char> message = {
            (uint8_t) Message::ReceiverMsgHeader::kBeginTransfer
    };
    queue_send_data(
            _sender_endpoint,
            message);
}

void Receiver::send_begin_block(int block_id) {
    spdlog::info(" Send Begin Block message, block id:{}", block_id);
    std::vector<unsigned char> message = {
            (uint8_t) Message::ReceiverMsgHeader::kBeginBlock,
    };
    std::vector<int> id = {block_id};
    auto &id_ = reinterpret_cast<std::vector<unsigned char>&>(id);
    message.insert(message.end(), id_.begin(), id_.end());
    queue_send_data(_sender_endpoint, message);
}

void Receiver::send_require_slice(int block_id, int slice_id) {
    spdlog::info(" Send Require Slice message, block id:{}, slice id:{}", block_id, slice_id);
    std::vector<unsigned char> message = {
            (uint8_t) Message::ReceiverMsgHeader::kRequireSlice
    };
    std::vector<int> ids = {block_id, slice_id};
    auto& ids_ = reinterpret_cast<std::vector<unsigned char>&>(ids);
    message.insert(message.end(), ids_.begin(), ids_.end());
    queue_send_data(_sender_endpoint, message);
}

void Receiver::send_end_transfer() {
    //End transfer
    spdlog::warn("Transmission of the file is finished, send end transfer message.");
    std::vector<unsigned char> message = {
            (uint8_t ) Message::ReceiverMsgHeader::kEndTransfer
    };

    queue_send_data(_sender_endpoint, message);
}

