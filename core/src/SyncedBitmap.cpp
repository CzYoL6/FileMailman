//
// Created by zaiyi on 2023/11/7.
//

#include <assert.h>

#include <core/SyncedBitmap.h>


SyncedBitmap::SyncedBitmap(int size, bool init_val) : _size(size){
    assert((size % 8 == 0) && size > 0);
    _bytes = new char[size / 8];
    uint8_t init_val_ = init_val ? 0xff : 0;
    for(int i = 0; i  < size / 8; i++)
        _bytes[i] = init_val_;
}

SyncedBitmap::~SyncedBitmap() {
    delete[] _bytes;
}

void SyncedBitmap::SetBit(int index, bool val) {
    assert(index < size * 8);

    std::lock_guard<std::mutex> l(_bitmap_mutex);

    if(val)
        _bytes[(index - 1) / 8] |= (1 << ((index - 1) % 8));
    else
        _bytes[(index - 1) / 8] &= ~(1 << ((index - 1) % 8));
}

bool SyncedBitmap::GetBit(int index)  {
    std::lock_guard<std::mutex> l(_bitmap_mutex);
    return (bool)((1 << ((index - 1) % 8)) & (_bytes[(index - 1) / 8]));
}

bool SyncedBitmap::IsFull()  {
    std::lock_guard<std::mutex> l(_bitmap_mutex);
    for(int i = 0; i < _size / 8; i++){
        if((uint8_t)_bytes[i] != 0xff) return false;
    }
    return true;
}

