//
// Created by zaiyichen on 2023/11/9.
//

#ifndef IMGUIOPENGL_NETWORKMANAGER_H
#define IMGUIOPENGL_NETWORKMANAGER_H

#include <string_view>
#include <cstdint>
#include <memory>
#include <asio.hpp>

class NetworkManager {
public:
    void Connect(std::string_view ip, uint16_t port);
    void Disconnect();
    void Tick();
    static NetworkManager& GetSingleton(){
        static NetworkManager instance;
        return instance;
    }
private:
    bool _bConnected;
    std::string _ip;
    uint16_t _port;
    asio::io_context _asio_io_context;
};


#endif //IMGUIOPENGL_NETWORKMANAGER_H
