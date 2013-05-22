
#include "libsms.h"
#include "../sms/system/interface.h"
#include "../sms/system/system.h"

class UserInterface : public _Interface {
public:
    smsAudioSampleCallback paudioSample;
    smsVideoRefreshCallback pvideoRefresh;
    smsInputPollCallback pinputPoll;
    smsInputStateCallback pinputState;

    void audioSample(signed sampleLeft, signed sampleRight, unsigned soundChip) {
        if (paudioSample) paudioSample(sampleLeft, sampleRight, soundChip);
    }
    void videoSample(u16* frame, u16 width, u16 height, Vdp::Mode mode, unsigned systemID) {
        if (pvideoRefresh) pvideoRefresh(frame, width, height, unsigned(mode), systemID != 1);
    }
    void pollInput() {
        if (pinputPoll) pinputPoll();
    }
    i16 getInputState(_Input::Port port, _Input::DeviceID deviceId, _Input::IObjectID iobjectId) {
        if (pinputState) return pinputState( (unsigned)port, (unsigned)deviceId, (unsigned)iobjectId );
        return 1;
    }
};

UserInterface interface;
_System _system(1);
_System _systemGG(2);

unsigned smsLibraryId() {
    return _system.getLibraryId();
}

void smsSetAudioSample(smsAudioSampleCallback smsAudioSample) {
    interface.paudioSample = smsAudioSample;
}
void smsSetVideoRefresh(smsVideoRefreshCallback smsVideoRefresh) {
    interface.pvideoRefresh = smsVideoRefresh;
}
void smsSetInputPoll(smsInputPollCallback smsInputPoll) {
    interface.pinputPoll = smsInputPoll;
}
void smsSetInputState(smsInputStateCallback smsInputState) {
    interface.pinputState = smsInputState;
}

void smsSetDevice(bool port, unsigned deviceId) {
    _system.setPortDevice((_Input::Port)port, (_Input::DeviceID)deviceId);
    _systemGG.setPortDevice((_Input::Port)port, (_Input::DeviceID)deviceId);
}

void smsPower() {
    _system.power();
    _systemGG.power();
}
void smsReset() {
    _system.reset();
    _systemGG.reset();
}
void smsRun() {
    _system.enter();
}

void smsLoad(const unsigned char* rom, unsigned romSize) {
    _system.init(&interface);
    _system.loadSMS((u8*)rom, romSize);
}
void smsLoadSG1000(const unsigned char* rom, unsigned romSize) {
    _system.init(&interface);
    _system.loadSG1000((u8*)rom, romSize);
}
void smsLoadGameGear(const unsigned char* rom, unsigned romSize, unsigned ggMode) {
    _system.init(&interface);
    _system.loadGG((u8*)rom, romSize, ggMode);
    if (ggMode != GG_MODE_GEAR_TO_GEAR) return;
    //gear-to-gear
    _systemGG.init(&interface);
    _systemGG.loadGG((u8*)rom, romSize, ggMode);
    _system.setFriendSystem( &_systemGG );
    _systemGG.setFriendSystem( &_system );
}
void smsLoadBios(const unsigned char* bios, unsigned biosSize) {
    _system.loadBios((u8*)bios, biosSize);
}

void smsUnload() {
    _system.unload();
    _systemGG.unload();
}

void smsSetRegion(unsigned regionId) {
    _system.setRegion((_System::Region)regionId);
    _systemGG.setRegion((_System::Region)regionId);
}
unsigned smsGetRegion() {
    return _system._region();
}
void smsSetRevision(unsigned revisionId) {
    _system.setRevision( (_System::Revision)revisionId);
}

void smsDisable3D(bool state) {
    _system.setDisable3D(state);
}

void smsEnableYamahaSoundChip(bool state) {
    _system.supportYamaha(state);
}

void smsResetCheat(void) {
    _system.resetCheat();
}

bool smsSetCheat(const char* code) {
    return _system.setCheat(code);
}

unsigned smsSavestateSize(void) {
    return _system.getSerializationSize();
}

unsigned long long smsSavestateTimeStamp(unsigned char* data) {
    return _system.getSerializationTimeStamp(data);
}

bool smsSaveState(unsigned char* data, unsigned size) {
    if (!_system.getSerializationData(data, size)) {
        return false;
    }
    return true;
}
bool smsLoadState(unsigned char* data, unsigned size) {
    return _system.setSerializationData(data, size);
}

unsigned char* smsGetMemoryData(unsigned memoryId) {
    if (!_system.cart.isLoaded()) return 0;
    if (memoryId == SMS_MEMORY_WRAM) return _system.getUnit() == _System::SG1000 ? _system.wramSG : _system.wram;
    if (memoryId == SMS_MEMORY_VRAM) return _system.vram;
    if (memoryId == SMS_MEMORY_SRAM) return _system.cart.getSram();
    return 0;
}
unsigned smsGetMemorySize(unsigned memoryId) {
    if (memoryId == SMS_MEMORY_WRAM) return _system.getUnit() == _System::SG1000 ? _system.wramSG.size() : _system.wram.size();
    if (memoryId == SMS_MEMORY_VRAM) return _system.vram.size();
    if (memoryId == SMS_MEMORY_SRAM) return _system.cart.getSramSize();
    return 0;
}

void smsDisableBorder(bool state) {
    _system.setDisableSmsBorder(state);
}
