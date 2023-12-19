//
// Created by zaiyichen on 2023/12/10.
//
#include <iostream>
#include <core/Sender.h>
#include <core/Receiver.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/async.h>


int main(int argc, char** argv){
    spdlog::warn("Launching...");

    try
    {
        if(strcmp(argv[1] ,"send") == 0) {
            std::shared_ptr<UdpClient> s = std::make_shared<Sender>(std::atoi(argv[2]), argv[3]);
            s->Start();
        }
        else if(strcmp(argv[1], "receive") == 0){
            std::shared_ptr<UdpClient> r = std::make_shared<Receiver>(argv[2], std::atoi(argv[3]));
            r->Start();
        }
        else{
            spdlog::error("wrong parameters!");
            exit(-1);
        }

    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    // flush and stop the logger (ensure all messages are processed)
    spdlog::shutdown();

    return 0;
}
