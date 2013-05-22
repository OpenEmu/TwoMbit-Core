
#ifndef MEM_H
#define MEM_H

#include <string.h>
#include "helper.h"
#include "dataTypes.h"

struct MemMap {
    enum Type { WRAM, WRAMSG, VRAM, ROM, BIOS, CARTWRAM, SRAM, MC, UNMAPPED } type;
	virtual u32 size() { return 0; }
	virtual u8 read(u32 addr) = 0;
	virtual void write(u32 addr, u8 value) = 0;
	bool isWriteProtected() { return false; }
};

struct Mmio {
	virtual u8 mmio_read(u8 pos) = 0;
	virtual void mmio_write(u8 pos, u8 value) = 0;
};

struct MemSystem : MemMap {
	void map(u8* source, u32 size) {
		data = source;
        data_size = data && size > 0 ? size : 0;
	}
	void write_protect(bool state) { write_protection = state; }
	u8* handle() { return (u8*)data; }
	u32 size() { return data_size; }
	bool isWriteProtected() { return write_protection; }
	u8 read(u32 addr) {
		return data[addr];
	}
	void write(u32 addr, u8 value) {
		if(write_protection) return;
		data[addr] = value;
	}
	void init(u8 init_value=0) { memset(data, init_value, data_size); }
	operator u8*() { return (u8*)data; }
	void operator=(u8* ptr) { data = ptr; }
	MemSystem(MemMap::Type _type) : data(0), data_size(0), write_protection(false) { type = _type; }
private:
	u8* data;
	u32 data_size;
	bool write_protection;
};

struct MemStatic : MemMap {
	u8 read(u32 addr) { return data[addr]; }
	void write(u32 addr, u8 value) { data[addr] = value; }
	u8* handle() { return (u8*)data; }
	u32 size() { return data_size; }
	void init(u8 init_value=0) { memset(data, init_value, data_size); }
	operator u8*() { return (u8*)data; }
	MemStatic(MemMap::Type _type, u32 size) : data_size(size) {
	    type = _type;
        data = new u8[data_size];
    }
	~MemStatic() { free_mem(data); }
private:
	u8 *data;
	u32 data_size;
};

#endif
