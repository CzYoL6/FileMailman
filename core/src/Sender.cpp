//
// Created by zaiyichen on 2023/11/29.
//

#include <iostream>
#include <core/Sender.h>
#include <spdlog/spdlog.h>
#include <core/Message.h>
#include <fstream>
#include <filesystem>

Sender::Sender(uint16_t port, std::string_view filename):
        UdpClient( port),
          _file_name(filename),
          _ifs(std::make_shared<std::ifstream >(filename.data())),
          _close_wait_timer(_io_context)
{

    _file_size = std::filesystem::file_size(filename);
    _current_buffer_block = std::make_unique<BufferBlock>(slice_size, block_size, _io_context);
    _block_count = (_file_size % block_size == 0) ? _file_size / block_size : _file_size / block_size + 1;
}


std::variant<std::span<char>, std::vector<unsigned char>> Sender::handle_data(std::span<char> data) {
    auto [clientMsgHeader, load] = Message::GetReceiverHeader(data);
    switch(clientMsgHeader){
        case Message::ReceiverMsgHeader::kBeginTransfer: {
            assert(load.empty());
            spdlog::info("Receiver Send Begin Transfer");

            std::vector<unsigned char> message = {
                    (uint8_t)Message::SenderMsgHeader::kBeginTransferAck_FileMeta,   //header
                    0x08, 0x00, 0x00,  0x00,   //8
                    0x74, 0x65, 0x73, 0x74, 0x2e, 0x70, 0x6e, 0x67,   //test.png
                    0x61, 0xbf, 0x91};
            return std::move(message);
            break;
        }
        case Message::ReceiverMsgHeader::kBeginBlock: {
            assert(load.size() == sizeof(int));
            int block_id = *(reinterpret_cast<int*>(load.data()));
            spdlog::info("Receiver Send Begin Block {}", block_id);

            load_block(block_id);

            std::vector<unsigned char> message = {
                    (uint8_t)Message::SenderMsgHeader::kBeginBlockAck
            };
            return std::move(message);
            break;
        }
        case Message::ReceiverMsgHeader::kRequireSlice: {
            assert(load.size() == sizeof(int));

            int slice_id = *(reinterpret_cast<int *>(load.data()));
            assert(slice_id < _current_buffer_block->slice_count());

            spdlog::info("Receiver Send Require Slice {}", slice_id);

            std::span<char> slice_data = _current_buffer_block->GetBlockSlice(slice_id)->data_span() ;

            return slice_data;

            break;
        }
        case Message::ReceiverMsgHeader::kEndTransfer: {
            assert(load.empty());
            spdlog::info("Receiver Send End Transfer");

            std::vector<unsigned char> message = {
                    (uint8_t)Message::SenderMsgHeader::kEndTransferAck
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

void Sender::load_block(int id) {
    if(_current_buffer_block_id != id){
        _current_buffer_block->ReadFromFile(*_ifs, id * block_size,
                                            std::min((uint64_t)block_size, _file_size - id * block_size));
    }
}

Sender::~Sender() {
    if(_ifs->is_open()){
        _ifs->close();
    }

}

int main(int argc, char** argv){
    try
    {

        Sender s(std::atoi(argv[1]), "test.png");

    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
