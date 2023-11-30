//
// Created by zaiyichen on 2023/11/30.
//

#ifndef IMGUIOPENGL_MESSAGE_H
#define IMGUIOPENGL_MESSAGE_H

#endif //IMGUIOPENGL_MESSAGE_H
#include <cstdint>

namespace Message {
    enum class ClientMsgHeader : uint8_t {
        kBeginTransfer = 1,
        kBeginBlock,
        kRequireSlice,
        kEndTransfer
    };

    enum class ServerMsgHeader : uint8_t {
        kBeginTransferAck_FileMeta = 1,
        kBeginBlockAck,
        kSliceData,
        kEndTransferAck
    };

    static std::tuple<ClientMsgHeader, std::span<char>> GetClientHeader(std::span<char> data){
        // header, load
        return {*reinterpret_cast<ClientMsgHeader*>(data.data()), std::span<char>(data.begin() + 1, data.end())};
    }
    static std::tuple<ServerMsgHeader, std::span<char>> GetServerHeader(std::span<char> data){
        // header, load
        return {*reinterpret_cast<ServerMsgHeader*>(data.data()), std::span<char>(data.begin() + 1, data.end())};
    }
}


