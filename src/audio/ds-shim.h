/*
 * Apple // emulator for *nix
 *
 * This software package is subject to the GNU General Public License
 * version 2 or later (your choice) as published by the Free Software
 * Foundation.
 *
 * THERE ARE NO WARRANTIES WHATSOEVER.
 *
 */

#ifndef _DS_SHIM_H_
#define _DS_SHIM_H_

#include "audio/win-shim.h"

// 2013/09/19 - http://msdn.microsoft.com/en-us/library/ms897820.aspx

typedef struct IDirectSoundBuffer {

    void *_this;

    int (*SetVolume)(void* _this, long lVolume);

    int (*GetVolume)(void* _this, LPLONG lplVolume);

    int (*GetCurrentPosition)(void* _this, LPDWORD lpdwCurrentPlayCursor, LPDWORD lpdwCurrentWriteCursor);

    int (*Stop)(void* _this);

    // This method restores the memory allocation for a lost sound buffer for the specified DirectSoundBuffer object.
    int (*Restore)(void *_this);

    int (*Play)(void* _this, unsigned long dwReserved1, unsigned long dwReserved2, unsigned long dwFlags);

    // This method obtains a valid write pointer to the sound buffer's audio data
    int (*Lock)(void* _this, unsigned long dwWriteCursor, unsigned long dwWriteBytes, LPVOID* lplpvAudioPtr1, LPDWORD lpdwAudioBytes1, LPVOID* lplpvAudioPtr2, LPDWORD lpdwAudioBytes2, unsigned long dwFlags);

    // This method releases a locked sound buffer.
    int (*Unlock)(void* _this, LPVOID lpvAudioPtr1, unsigned long dwAudioBytes1, LPVOID lpvAudioPtr2, unsigned long dwAudioBytes2);

    int (*GetStatus)(void* _this, LPDWORD lpdwStatus);

    int (*UnlockStaticBuffer)(void* _this, unsigned long dwAudioBytes);

    int (*Replay)(void* _this);

} IDirectSoundBuffer, *LPDIRECTSOUNDBUFFER, **LPLPDIRECTSOUNDBUFFER;

#define DS_OK                           0

// The call failed because resources (such as a priority level)
// were already being used by another caller.
#define DSERR_ALLOCATED                 0x8878000A

// The control (vol,pan,etc.) requested by the caller is not available.
#define DSERR_CONTROLUNAVAIL            0x8878001E

// An invalid parameter was passed to the returning function
#define DSERR_INVALIDPARAM              0x80070057

// This call is not valid for the current state of this object
#define DSERR_INVALIDCALL               0x88780032

// An undetermined error occured inside the DirectSound subsystem
#define DSERR_GENERIC                   0x80004005

// The caller does not have the priority level required for the function to
// succeed.
#define DSERR_PRIOLEVELNEEDED           0x88780046

// Not enough free memory is available to complete the operation
#define DSERR_OUTOFMEMORY               0x8007000E

// The specified WAVE format is not supported
#define DSERR_BADFORMAT                 0x88780064

// The function called is not supported at this time
#define DSERR_UNSUPPORTED               0x80004001

// No sound driver is available for use
#define DSERR_NODRIVER                  0x88780078

// This object is already initialized
#define DSERR_ALREADYINITIALIZED        0x88780082

// This object does not support aggregation
#define DSERR_NOAGGREGATION             0x80040110

// The buffer memory has been lost, and must be restored.
#define DSERR_BUFFERLOST                0x88780096

// Another app has a higher priority level, preventing this call from
// succeeding.
#define DSERR_OTHERAPPHASPRIO           0x887800A0

// This object has not been initialized
#define DSERR_UNINITIALIZED             0x887800AA

// The requested COM interface is not available
#define DSERR_NOINTERFACE               0x80000004

// Access is denied
#define DSERR_ACCESSDENIED              0x80070005

// Tried to create a DSBCAPS_CTRLFX buffer shorter than DSBSIZE_FX_MIN milliseconds
#define DSERR_BUFFERTOOSMALL            0x887800B4

// Attempt to use DirectSound 8 functionality on an older DirectSound object
#define DSERR_DS8_REQUIRED              0x887800BE

// A circular loop of send effects was detected
#define DSERR_SENDLOOP                  0x887800C8

// The GUID specified in an audiopath file does not match a valid MIXIN buffer
#define DSERR_BADSENDBUFFERGUID         0x887800D2

// The object requested was not found (numerically equal to DMUS_E_NOT_FOUND)
#define DSERR_OBJECTNOTFOUND            0x88781193


#define DSBLOCK_FROMWRITECURSOR 0x1 // Locks from the current write position, making a call to IDirectSoundBuffer::GetCurrentPosition unnecessary. If this flag is specified, the dwWriteCursor parameter is ignored.
#define DSBLOCK_ENTIREBUFFER    0x2 // Locks the entire buffer. The dwWriteBytes parameter is ignored.


#define     DSBSTATUS_BUFFERLOST   0x00000002
#define     DSBSTATUS_LOOPING   0x00000004
#define     DSBSTATUS_PLAYING   0x00000001
#define     _DSBSTATUS_NOTPLAYING 0x08000000
#define DSBPLAY_LOOPING             0x00000001
#define DSBVOLUME_MIN               -10000
#define DSBVOLUME_MAX               0

#define DSSCL_NORMAL                0x00000001

#if defined(FAILED)
#undef FAILED
#endif
#if defined(SUCCEEDED)
#undef SUCCEEDED
#endif
static inline bool FAILED(int x) { return x != DS_OK; }
static inline bool SUCCEEDED(int x) { return x == DS_OK; }

#define WAVE_FORMAT_PCM 0x0001
#define DSBCAPS_GETCURRENTPOSITION2 0x00010000
#define DSBCAPS_STICKYFOCUS         0x00004000
#define DSBCAPS_LOCSOFTWARE         0x00000008
#define DSBCAPS_CTRLVOLUME          0x00000080
#define DSBCAPS_CTRLPOSITIONNOTIFY  0x00000100


typedef struct {
    uint16_t  wFormatTag;
    uint16_t  nChannels;
    unsigned long nSamplesPerSec;
    unsigned long nAvgBytesPerSec;
    uint16_t  nBlockAlign;
    uint16_t  wBitsPerSample;
    uint16_t  cbSize;
} WAVEFORMATEX, *LPWAVEFORMATEX;

typedef struct DSBUFFERDESC { 
    unsigned long dwSize; 
    unsigned long dwFlags; 
    unsigned long dwBufferBytes; 
    unsigned long dwReserved; 
    LPWAVEFORMATEX lpwfxFormat; 
} DSBUFFERDESC; 

typedef unsigned long DSCAPS_MASK;
typedef struct _DSCAPS
{
    unsigned long           dwSize;
    DSCAPS_MASK     dwFlags;
    unsigned long           dwMinSecondarySampleRate;
    unsigned long           dwMaxSecondarySampleRate;
    unsigned long           dwPrimaryBuffers;
    unsigned long           dwMaxHwMixingAllBuffers;
    unsigned long           dwMaxHwMixingStaticBuffers;
    unsigned long           dwMaxHwMixingStreamingBuffers;
    unsigned long           dwFreeHwMixingAllBuffers;
    unsigned long           dwFreeHwMixingStaticBuffers;
    unsigned long           dwFreeHwMixingStreamingBuffers;
    unsigned long           dwMaxHw3DAllBuffers;
    unsigned long           dwMaxHw3DStaticBuffers;
    unsigned long           dwMaxHw3DStreamingBuffers;
    unsigned long           dwFreeHw3DAllBuffers;
    unsigned long           dwFreeHw3DStaticBuffers;
    unsigned long           dwFreeHw3DStreamingBuffers;
    unsigned long           dwTotalHwMemBytes;
    unsigned long           dwFreeHwMemBytes;
    unsigned long           dwMaxContigFreeHwMemBytes;
    unsigned long           dwUnlockTransferRateHwBuffers;
    unsigned long           dwPlayCpuOverheadSwBuffers;
    unsigned long           dwReserved1;
    unsigned long           dwReserved2;
} DSCAPS, *LPDSCAPS;
typedef DSCAPS *LPCDSCAPS;

typedef struct IDirectSound {

    void *implementation_specific;

#define LPUNKNOWN void*
    int (*CreateSoundBuffer)(DSBUFFERDESC *pcDSBufferDesc, LPDIRECTSOUNDBUFFER * ppDSBuffer, LPUNKNOWN pUnkOuter);
    int (*DestroySoundBuffer)(LPDIRECTSOUNDBUFFER * ppDSBuffer);

} IDirectSound, *LPDIRECTSOUND;

#endif /* whole file */

