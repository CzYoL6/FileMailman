//
// Created by zaiyichen on 2023/12/10.
//
#include <iostream>
#include <core/Sender.h>
#include <core/Receiver.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <app/progressbar.hpp>

tqdm bar;
void finish_progress_bar(){
    bar.finish();
}
void update_progress_bar(int v, int max){
    bar.progress(v, max);
}

int main(int argc, char** argv){
#ifdef WL_RELEASE
    spdlog::set_level(spdlog::level::critical);
    #define SHOW_PROGRESS_BAR
#endif
    spdlog::warn("Launching...");
    boost::asio::io_context ioContext;
    try
    {
        if(strcmp(argv[1] ,"send") == 0) {
            std::shared_ptr<UdpClient> s = std::make_shared<Sender>(ioContext, std::atoi(argv[2]), argv[3]);
            s->Start();
        }
        else if(strcmp(argv[1], "receive") == 0){
            std::shared_ptr<UdpClient> r = std::make_shared<Receiver>(ioContext,
                                                                      argv[2],
                                                                      std::atoi(argv[3]),
                                                                      &finish_progress_bar,
                                                                      &update_progress_bar);
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

    return 0;
}
