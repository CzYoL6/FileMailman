//
// Created by zaiyichen on 2023/11/29.
//

#include <core/Receiver.h>
#include <core/Message.h>
#include <spdlog/spdlog.h>
#include <iostream>
#include <fstream>

Receiver::Receiver(boost::asio::io_context &io_context, std::string_view ip, uint16_t port,
                   const std::function<void()> &finish_progress_cb,
                   const std::function<void(int, int)> &update_progress_cb)
    : UdpClient(io_context),
      _sender_endpoint(boost::asio::ip::address::from_string(ip.data()), port),
      _file_size(0),
      _block_count(0),
      _current_buffer_block(std::make_unique<BufferBlock>(0, 0 , 0)),
      _update_progress_cb(update_progress_cb),
      _finish_progress_cb(finish_progress_cb)

{
    spdlog::warn("client launched. ");

    _socket.connect(_sender_endpoint);

    // send begin transfer message
    auto m = generate_begin_transfer();
    queue_send_data(m.endpoint, std::move(m.data), kReliable, 0); // 0, do not need to ack any message
}

Receiver::~Receiver() {
    if(_ofs && _ofs->is_open()){
        _ofs->close();
    }
}

Receiver::HandleDataRetValue Receiver::handle_data(boost::asio::ip::udp::endpoint endpoint, std::span<char> data) {
    Receiver::HandleDataRetValue responses;
    if(!_running) return responses;
    spdlog::info("handling data...");
    auto [senderMsgHeader, load] = Message::GetSenderHeader(data);
    switch(senderMsgHeader) {
        case Message::SenderMsgHeader::kBeginTransferAck_FileMeta: {
            // start total time timer
            _time_point = _total_time_timer.now();

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
            _current_buffer_block = std::make_unique<BufferBlock>(_current_block_id, slice_size, real_block_size);

            responses.emplace_back(generate_begin_block(0));
            break;
        }
        case Message::SenderMsgHeader::kBeginBlockAck:{
            int block_id = *(reinterpret_cast<int*>(load.data()));
            spdlog::info("Message Begin Block is acked, block id: {}", block_id);

            if(block_id != _current_block_id){
                break;
            }

            // ask all the slices in the block
            for(int i = 0; i < _current_buffer_block->slice_count(); i++) {
                responses.emplace_back(generate_require_slice(_current_block_id, i));
            }
            break;
        }
        case Message::SenderMsgHeader::kSliceData: {
            int block_id = *(reinterpret_cast<int *>(load.data()));
            assert(block_id < _block_count);

            int slice_id = *(reinterpret_cast<int *>(load.data() + 4));
            assert(slice_id < _current_buffer_block->slice_count());

            spdlog::info("Receive slice data of slice {} ({} in total), block {} ({} in total)",
                         slice_id,
                         _current_buffer_block->slice_count() - 1,
                         block_id,
                         _block_count - 1
                         );

            if(block_id != _current_block_id){
                spdlog::warn("Not current block id, redundant packet detected!");
                break;
            }


            if(_current_buffer_block->IsSliceDone(slice_id)){
                spdlog::warn("Slice {} of block {} has already been received!");
                break;
            }

            std::span<char> slice_data(load.begin() + 8, load.end());
            _current_buffer_block->GetBlockSlice(slice_id)->WriteIn(slice_data.data(), slice_data.size());
            _current_buffer_block->SetSliceDone(slice_id);

            // check if all slices are received
            if(_current_buffer_block->SliceAllDone()){
                block_finish(_current_block_id);

                // require next block
                _current_block_id++;
                if(_current_block_id == _block_count){
                    responses.emplace_back(generate_end_transfer());
                    break;
                }

                int real_block_size = std::min((int)block_size, (int)(_file_size - _current_block_id * block_size));
                _current_buffer_block = std::make_unique<BufferBlock>(_current_block_id, slice_size, real_block_size);

                responses.emplace_back(generate_begin_block(_current_block_id));
                break;
            }

            break;

        }
        case Message::SenderMsgHeader::kEndTransferAck:{
            spdlog::warn("Message End Transfer is acked, ready to end program");
            auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(_total_time_timer.now() - _time_point);
            spdlog::critical("Total time: {} ms, Average Speed: {} KB/s",
                             total_time.count(),
                             _file_size / 1024.0/ (total_time.count() / 1000.0));
            clear_acking_list();
            _running = false;
            _ofs->close();
            _finish_progress_cb();
            _io_context.stop();
            break;
        }
        default: {
            spdlog::error("Message parsing failed: wrong header value {}", (uint8_t)senderMsgHeader);
            exit(-1);
        }
    }
    return responses;
}

void Receiver::save_block(int id) {
    if(!_running) return;
    int bytes_size = std::min((uint64_t)block_size, _file_size - id * block_size);
    _current_buffer_block->WriteToFile(*_ofs, id * block_size, bytes_size);
    spdlog::info("Save {} bytes' data of block {} (max {}) ", bytes_size, id, _block_count - 1);
}

Receiver::HandleDataRetItem Receiver::generate_begin_transfer() {
    spdlog::info("Send Begin Transfer message.");
    std::vector<unsigned char> message = {
            (uint8_t) Message::ReceiverMsgHeader::kBeginTransfer
    };
    return {_sender_endpoint, std::move(message), kReliable};
}

Receiver::HandleDataRetItem Receiver::generate_begin_block(int block_id) {
    spdlog::info(" Send Begin Block message, block id:{}", block_id);
    std::vector<unsigned char> message = {
            (uint8_t) Message::ReceiverMsgHeader::kBeginBlock,
    };
    std::vector<int> id = {block_id};
    auto &id_ = reinterpret_cast<std::vector<unsigned char>&>(id);
    message.insert(message.end(), id_.begin(), id_.end());
    return {_sender_endpoint, std::move(message), kReliable};
}

Receiver::HandleDataRetItem
Receiver::generate_require_slice(int block_id, int slice_id) {
    spdlog::info(" Send Require Slice message, block id:{}, slice id:{}", block_id, slice_id);
    std::vector<unsigned char> message = {
            (uint8_t) Message::ReceiverMsgHeader::kRequireSlice
    };
    std::vector<int> ids = {block_id, slice_id};
    auto& ids_ = reinterpret_cast<std::vector<unsigned char>&>(ids);
    message.insert(message.end(), ids_.begin(), ids_.end());
    return {_sender_endpoint, std::move(message), kReliable};
}

Receiver::HandleDataRetItem Receiver::generate_end_transfer() {
    //End transfer
    spdlog::warn("Transmission of the file is finished, send end transfer message.");
    std::vector<unsigned char> message = {
            (uint8_t ) Message::ReceiverMsgHeader::kEndTransfer
    };
    return {_sender_endpoint, std::move(message), kReliable};
}

void Receiver::block_finish(int block_id) {
    spdlog::warn("Transmission of block {} is finished, saving data.", _current_block_id);
    // save the block
    save_block(_current_block_id);

    _update_progress_cb(block_id, _block_count);

}
