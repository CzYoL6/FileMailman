//
// Created by zaiyi on 2023/11/7.
//

#ifndef IMGUIOPENGL_SYNCEDBITMAP_H
#define IMGUIOPENGL_SYNCEDBITMAP_H

#include <mutex>

class SyncedBitmap {
public:
    SyncedBitmap(int size, bool init_val);
    ~SyncedBitmap();

public:
    void SetBit(int index, bool val);
    bool GetBit(int index) ;
    bool IsFull() ;

public:
    int size() const {return _size;};

private:
    int _size;
    char* _bytes;
    std::mutex _bitmap_mutex;
};


#endif //IMGUIOPENGL_SYNCEDBITMAP_H
