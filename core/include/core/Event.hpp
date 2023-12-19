//
// Created by zaiyichen on 2023/12/19.
//

#ifndef FILEMAILMAN_EVENT_H
#define FILEMAILMAN_EVENT_HPP
#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>

template<typename... F>
class Event
{
public:
    // Subscribe a function to the event
    void Subscribe(std::function<void(F...)> handler)
    {
        handlers.push_back(handler);
    }

    // Trigger the event, calling all subscribed functions
    template<typename... Args>
    void Trigger(Args&&... args)
    {
        for (auto handler : handlers)
        {
            handler(std::forward<Args>(args)...);
        }
    }


private:
    std::vector<std::function<void(F...)>> handlers;
};


#endif //FILEMAILMAN_EVENT_H
