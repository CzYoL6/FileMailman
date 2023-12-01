//
// Created by zaiyichen on 2023/11/30.
//

#ifndef IMGUIOPENGL_MESSAGE_H
#define IMGUIOPENGL_MESSAGE_H

#endif //IMGUIOPENGL_MESSAGE_H
#include <cstdint>

namespace Message {
    enum class ReceiverMsgHeader : uint8_t {
        kBeginTransfer = 1,
        kBeginBlock,
        kRequireSlice,
        kEndTransfer
    };

    enum class SenderMsgHeader : uint8_t {
        kBeginTransferAck_FileMeta = 1,
        kBeginBlockAck,
        kSliceData,
        kEndTransferAck
    };

    static std::tuple<ReceiverMsgHeader, std::span<char>> GetReceiverHeader(std::span<char> data){
        // header, load
        return {*reinterpret_cast<ReceiverMsgHeader*>(data.data()), std::span<char>(data.begin() + 1, data.end())};
    }
    static std::tuple<SenderMsgHeader, std::span<char>> GetSenderHeader(std::span<char> data){
        // header, load
        return {*reinterpret_cast<SenderMsgHeader*>(data.data()), std::span<char>(data.begin() + 1, data.end())};
    }
}


