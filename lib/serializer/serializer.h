#ifndef SERIALIZER_H
#define SERIALIZER_H

#include "dataTypes.h"
#include "helper.h"

#include <cstring>
using namespace std;

class Serializer {
    enum Mode { Load, Save, Size };
    Mode _mode;
    u8* _data;
    unsigned _size;
    unsigned _capacity;

public:

    const u8* handle() const {
        return _data;
    }

    unsigned size() const {
        return _size;
    }

    template<typename T> void integer(T &value) {
        enum { size = sizeof(T) };
        if(_mode == Save) {
            for(unsigned n = 0; n < size; n++) _data[_size++] = value >> (n << 3);
        } else if(_mode == Load) {
            value = 0;
            for(unsigned n = 0; n < size; n++) {
                value |= _data[_size++] << (n << 3);
            }
        } else if(_mode == Size) {
            _size += size;
        }
    }

    template<typename T> void array(T array, unsigned size) {
        for(unsigned n = 0; n < size; n++) integer(array[n]);
    }

    Serializer() {
        _mode = Size;
        _data = 0;
        _size = 0;
    }

    Serializer(unsigned capacity) {
        _mode = Save;
        _data = new u8[capacity];
        _size = 0;
        _capacity = capacity;
    }

    Serializer(const u8 *data, unsigned capacity) {
        _mode = Load;
        _data = new u8[capacity];
        _size = 0;
        _capacity = capacity;
        memcpy(_data, data, capacity);
    }

    ~Serializer() {
        free_mem(_data);
    }
};

#endif
