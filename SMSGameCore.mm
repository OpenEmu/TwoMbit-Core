/*
 Copyright (c) 2013, OpenEmu Team
 

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
     * Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
     * Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
     * Neither the name of the OpenEmu Team nor the
       names of its contributors may be used to endorse or promote products
       derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY OpenEmu Team ''AS IS'' AND ANY
 EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL OpenEmu Team BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "SMSGameCore.h"
#import "OESMSSystemResponderClient.h"
#import "OEGGSystemResponderClient.h"
#import <OpenEmuBase/OERingBuffer.h>
#import <OpenGL/gl.h>

#import "libsms.h"
#import "color.h"
#import "resampler.h"

enum systemTypes{ SMS, GG };

@interface SMSGameCore () <OESMSSystemResponderClient, OEGGSystemResponderClient>
{
    int           systemType;
    uint16_t      controllerMask1;
    uint16_t      controllerMask2;
    bool          systemPause;
    int           width;
    int           height;
    Color         pixelFormatConverter;
    Resampler     resampler;
    NSData       *rom;
    NSString     *romName;
    unsigned int *videoBuffer;
}
@end

@implementation SMSGameCore

static __weak SMSGameCore *_current;

#pragma mark Callbacks

void videoCallback(const unsigned short* frame, unsigned width, unsigned height, unsigned modeId, bool secondGG)
{
    GET_CURRENT_OR_RETURN();

    current->width = width;
    current->height = height;
    
    unsigned gpu_pitch = width * sizeof(unsigned int);
    current->pixelFormatConverter.update();
    current->pixelFormatConverter.active_filter->render((Color::Mode)modeId, current->videoBuffer, gpu_pitch, (u16*)frame, width, height);
}

static void audioCallback(signed sampleLeft, signed sampleRight, unsigned soundChip)
{
    GET_CURRENT_OR_RETURN();

    if(soundChip == SMS_SOUND_SN76489)
    {
        [[current ringBufferAtIndex:0] write:&sampleLeft maxLength:2];
        [[current ringBufferAtIndex:0] write:&sampleRight maxLength:2];
    }
    else if(soundChip == SMS_SOUND_YM2413)
    {   
        signed samples[] = { sampleLeft, sampleRight };
        current->resampler.sample(samples);
        while(current->resampler.pending()) {
            current->resampler.read(samples);
            [[current ringBufferAtIndex:1] write:&samples[0] maxLength:2];
            [[current ringBufferAtIndex:1] write:&samples[1] maxLength:2];
        }
    }
    else
    {
        [[current ringBufferAtIndex:1] write:&sampleLeft maxLength:2];
        [[current ringBufferAtIndex:1] write:&sampleRight maxLength:2];
    }
}

static signed inputCallback (unsigned port, unsigned deviceId, unsigned objectId)
{
    GET_CURRENT_OR_RETURN(0);

    if(deviceId == SMS_DEVICE_JOYPAD)
    {
        if(port == SMS_PORT_1)
            return (current->controllerMask1 >> objectId);
        else
            return (current->controllerMask2 >> objectId);
    }
    else if(deviceId == SMS_DEVICE_MISC && objectId == SMS_INPUT_PAUSE)
    {
        return current->systemPause;
    }

    return 0;
}

#pragma mark Core Implementation

- (id)init
{
    self = [super init];
    if(self != nil)
    {
        videoBuffer     = (unsigned int *) malloc(262 * 240 * sizeof(unsigned int));

        controllerMask1 = 0;
        controllerMask2 = 0;
        
        _current = self;
    }
    return self;
}

- (void)dealloc
{
    free(videoBuffer);
}

- (void)executeFrame
{
    smsRun();
}

- (void)setupEmulation
{
    smsSetVideoRefresh(videoCallback);
    smsSetAudioSample(audioCallback);
    smsSetInputState(inputCallback);

    smsSetDevice(SMS_PORT_1, SMS_DEVICE_JOYPAD);
    smsSetDevice(SMS_PORT_2, SMS_DEVICE_JOYPAD);

    smsEnableYamahaSoundChip(true);
    resampler.setFrequency(49716.0, 44100.0);
}

- (BOOL)loadFileAtPath:(NSString *)path error:(NSError **)error
{
    NSString *systemIdentifier = [self systemIdentifier];

    if([systemIdentifier isEqualToString:@"openemu.system.sms"])
    {
        systemType = SMS;
        width      = 256;
        height     = 225;
    }
    else if([systemIdentifier isEqualToString:@"openemu.system.gg"])
    {
        systemType = GG;
        width      = 160;
        height     = 144;
    }

    romName = [path copy];
    rom = [NSData dataWithContentsOfFile:[romName stringByStandardizingPath]];
    if(rom == nil) return NO;

    unsigned int size = [rom length];
    uint8_t *data = (uint8_t *)[rom bytes];

    if(systemType == SMS)
        smsLoad(data, size);
    if(systemType == GG)
        smsLoadGameGear(data, size, GG_MODE_NORMAL);
    
    smsPower();

    NSString *extensionlessFilename = [[romName lastPathComponent] stringByDeletingPathExtension];
    NSString *batterySavesDirectory = [self batterySavesDirectoryPath];

    if([batterySavesDirectory length] != 0)
    {
        [[NSFileManager defaultManager] createDirectoryAtPath:batterySavesDirectory withIntermediateDirectories:YES attributes:nil error:NULL];
        NSString *filePath = [batterySavesDirectory stringByAppendingPathComponent:[extensionlessFilename stringByAppendingPathExtension:@"sav"]];

        FILE *batteryFile = fopen([filePath UTF8String], "rb");

        if(batteryFile)
        {
            unsigned int   memorySize = smsGetMemorySize(SMS_MEMORY_SRAM);
            unsigned char *memoryData = smsGetMemoryData(SMS_MEMORY_SRAM);

            if(fread(memoryData, sizeof(unsigned char), memorySize, batteryFile) != memorySize)
                NSLog(@"Did not load SRAM properly");
        }

        fclose(batteryFile);
    }

    return YES;
}

- (void)resetEmulation
{
    smsReset();
}

- (void)stopEmulation
{
    NSString *extensionlessFilename = [[romName lastPathComponent] stringByDeletingPathExtension];
    NSString *batterySavesDirectory = [self batterySavesDirectoryPath];

    if([batterySavesDirectory length] != 0)
    {
        [[NSFileManager defaultManager] createDirectoryAtPath:batterySavesDirectory withIntermediateDirectories:YES attributes:nil error:NULL];
        NSString *filePath = [batterySavesDirectory stringByAppendingPathComponent:[extensionlessFilename stringByAppendingPathExtension:@"sav"]];

        unsigned int   memorySize = smsGetMemorySize(SMS_MEMORY_SRAM);
        unsigned char *memoryData = smsGetMemoryData(SMS_MEMORY_SRAM);

        FILE  *batteryFile  = fopen([filePath UTF8String], "wb");
        if(fwrite(memoryData, sizeof(unsigned char), memorySize, batteryFile) != memorySize)
            NSLog(@"Did not save SRAM properly");
        
        fclose(batteryFile);
    }

    smsUnload();
    
    [super stopEmulation];
}

#pragma mark Video

- (OEIntRect)screenRect
{
    return OEIntRectMake(0, 0, width, height);
}

- (OEIntSize)bufferSize
{
    return systemType == SMS ? OEIntSizeMake(262, 240) : OEIntSizeMake(160, 144);
}

- (OEIntSize)aspectSize
{
    return systemType == GG ? OEIntSizeMake(10, 9) : OEIntSizeMake(4, 3);
}

- (const void *)videoBuffer
{
    return videoBuffer;
}

- (GLenum)pixelFormat
{
    return GL_BGRA;
}

- (GLenum)pixelType
{
    return GL_UNSIGNED_INT_8_8_8_8_REV;
}

- (GLenum)internalPixelFormat
{
    return GL_RGB8;
}

#pragma mark Audio

- (NSUInteger)audioBufferCount
{
    return 2;
}

- (double)audioSampleRateForBuffer:(NSUInteger)buffer
{
    if(buffer == 0)
        return 44135;
    else
        return 44157;
}

- (NSUInteger)channelCountForBuffer:(NSUInteger)buffer
{
    return 2;
}

#pragma mark Save States

- (NSData *)serializeStateWithError:(NSError **)outError
{
    size_t length = smsSavestateSize();
    unsigned char *bytes = (unsigned char *)malloc(length);
    
    if(smsSaveState(bytes, length))
        return [NSData dataWithBytesNoCopy:(void *)bytes length:length];

    if(outError) {
        *outError = [NSError errorWithDomain:OEGameCoreErrorDomain code:OEGameCoreCouldNotSaveStateError userInfo:@{
            NSLocalizedDescriptionKey : @"Save state data could not be written",
            NSLocalizedRecoverySuggestionErrorKey : @"The emulator could not write the state data."
        }];
    }
    
    return nil;
}

- (BOOL)deserializeState:(NSData *)state withError:(NSError **)outError
{
    size_t saveStateSize = smsSavestateSize();
    if(saveStateSize != [state length]) {
        if(outError) {
            *outError = [NSError errorWithDomain:OEGameCoreErrorDomain code:OEGameCoreStateHasWrongSizeError userInfo:@{
                NSLocalizedDescriptionKey : @"Save state data has the wrong size.",
                NSLocalizedRecoverySuggestionErrorKey : [NSString stringWithFormat:@"The size of the save state does not have the right size, %ld expected, got: %ld.", saveStateSize, [state length]]
            }];
        }
        
        return NO;
    }

    if(smsLoadState((unsigned char *)[state bytes], saveStateSize))
        return YES;

    if(outError) {
        *outError = [NSError errorWithDomain:OEGameCoreErrorDomain code:OEGameCoreCouldNotLoadStateError userInfo:@{
            NSLocalizedDescriptionKey : @"The save state data could not be read",
            NSLocalizedRecoverySuggestionErrorKey : @"Could not read the save state data."
        }];
    }

    return NO;
}

- (void)saveStateToFileAtPath:(NSString *)fileName completionHandler:(void (^)(BOOL, NSError *))block
{
    int saveStateSize = smsSavestateSize();
    NSMutableData *stateData = [NSMutableData dataWithLength:saveStateSize];

    if(!smsSaveState((unsigned char *)[stateData mutableBytes], saveStateSize)) {
        NSError *error = [NSError errorWithDomain:OEGameCoreErrorDomain code:OEGameCoreCouldNotSaveStateError userInfo:@{
            NSLocalizedDescriptionKey : @"Save state data could not be written",
            NSLocalizedRecoverySuggestionErrorKey : @"The emulator could not write the state data."
        }];
        block(NO, error);
        return;
    }

    NSError *error = nil;
    BOOL success = [stateData writeToFile:fileName options:NSDataWritingAtomic error:&error];
    block(success, success ? nil : error);
}

- (void)loadStateFromFileAtPath:(NSString *)fileName completionHandler:(void (^)(BOOL, NSError *))block
{
    NSError *error;
    NSData *data = [NSData dataWithContentsOfFile:fileName options:NSDataReadingMappedIfSafe | NSDataReadingUncached error:&error];

    if(data == nil) {
        block(NO, error);
        return;
    }

    int saveStateSize = smsSavestateSize();
    if(saveStateSize != [data length]) {
        NSError *error = [NSError errorWithDomain:OEGameCoreErrorDomain code:OEGameCoreStateHasWrongSizeError userInfo:@{
            NSLocalizedDescriptionKey : @"Save state has wrong file size.",
            NSLocalizedRecoverySuggestionErrorKey : [NSString stringWithFormat:@"The size of the file %@ does not have the right size, %d expected, got: %ld.", fileName, saveStateSize, [data length]],
        }];
        block(NO, error);
        return;
    }

    if(!smsLoadState((unsigned char *)[data bytes], saveStateSize)) {
        NSError *error = [NSError errorWithDomain:OEGameCoreErrorDomain code:OEGameCoreCouldNotLoadStateError userInfo:@{
            NSLocalizedDescriptionKey : @"The save state data could not be read",
            NSLocalizedRecoverySuggestionErrorKey : [NSString stringWithFormat:@"Could not read the file state in %@.", fileName]
        }];
        block(NO, error);
        return;
    }

    block(YES, nil);
}

#pragma mark Input

- (oneway void)didPushSMSButton:(OESMSButton)button forPlayer:(NSUInteger)player
{
    if(player == 1)
        controllerMask1 |= 1 << button;
    else
        controllerMask2 |= 1 << button;
}

- (oneway void)didReleaseSMSButton:(OESMSButton)button forPlayer:(NSUInteger)player
{
    if(player == 1)
        controllerMask1 &= ~(1 << button);
    else
        controllerMask2 &= ~(1 << button);
}

- (oneway void)didPushSMSResetButton
{
}

- (oneway void)didReleaseSMSResetButton
{
}

- (oneway void)didPushSMSStartButton
{
    systemPause = true;
}

- (oneway void)didReleaseSMSStartButton
{
    systemPause = false;
}

- (oneway void)didPushGGButton:(OEGGButton)button
{
    controllerMask1 |= 1 << button;
}

- (oneway void)didReleaseGGButton:(OEGGButton)button
{
    controllerMask1 &= ~(1 << button);
}

@end
