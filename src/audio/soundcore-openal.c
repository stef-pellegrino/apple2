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

// soundcore OpenAL backend -- streaming audio

#include "common.h"

#ifdef __APPLE__
#   import <OpenAL/al.h>
#   import <OpenAL/alc.h>
#else
#   include <AL/al.h>
#   include <AL/alc.h>
#   include <AL/alext.h>
#endif

#include "audio/alhelpers.h"

#define OPENAL_NUM_BUFFERS 4

typedef struct ALPlayBuf {
    const ALuint bufid; // the hash id
    ALuint bytes;       // bytes to play
    UT_hash_handle hh;  // make this struct hashable
    struct ALPlayBuf *_avail_next;
} ALPlayBuf;

typedef struct ALVoice {
    ALuint source;

    // playing data
    ALPlayBuf *queued_buffers;
    ALint _queued_total_bytes; // a maximum estimate -- actual value depends on OpenAL query

    // working data buffer
    ALbyte *data;
    ALsizei index;      // working buffer byte index
    ALsizei buffersize; // working buffer size (and OpenAL buffersize)

    // available buffers
    ALPlayBuf *avail_buffers;

    ALsizei replay_index;

    // sample parameters
    ALenum format;
    ALenum channels;
    ALenum type;
    ALuint rate;
} ALVoice;

typedef struct ALVoices {
    const ALuint source;
    ALVoice *voice;
    UT_hash_handle hh;
} ALVoices;

static ALVoices *voices = NULL;

static AudioBackend_s openal_audio_backend = { 0 };

static long OpenALCreateSoundBuffer(const AudioParams_s *params, INOUT AudioBuffer_s **soundbuf_struct, const AudioContext_s *audio_context);
static long OpenALDestroySoundBuffer(INOUT AudioBuffer_s **soundbuf_struct);

// ----------------------------------------------------------------------------
// uthash of OpenAL buffers

static ALPlayBuf *PlaylistEnqueue(ALVoice *voice, ALuint bytes)
{
    ALPlayBuf *node = voice->avail_buffers;
    if (node == NULL)
    {
        ERRLOG("OOPS, sound playback overflow!");
        return NULL;
    }
    //LOG("Really enqueing OpenAL buffer %u", node->bufid);

    ALPlayBuf *node2 = NULL;
    HASH_FIND_INT(voice->queued_buffers, &node->bufid, node2);
    if (node2 != NULL)
    {
        ERRLOG("OOPS, confused ... ALPlayBuf already added!");
        return NULL;
    }

    // remove from list / place on hash
    voice->avail_buffers = node->_avail_next;
    node->_avail_next = NULL;
    HASH_ADD_INT(voice->queued_buffers, bufid, node);

    node->bytes = bytes;

    voice->_queued_total_bytes += node->bytes;
    voice->index = 0;
    assert(voice->_queued_total_bytes > 0);

#if 0
    ALPlayBuf *tmp = voice->queued_buffers;
    unsigned int count = HASH_COUNT(voice->queued_buffers);
    LOG("\t(numqueued: %d)", count);
    for (unsigned int i = 0; i < count; i++, tmp = tmp->hh.next)
    {
        LOG("\t(bufid : %u)", tmp->bufid);
    }
#endif

    return node;
}

static ALPlayBuf *PlaylistGet(ALVoice *voice, ALuint bufid)
{
    ALPlayBuf *node = NULL;
    HASH_FIND_INT(voice->queued_buffers, &bufid, node);
    return node;
}

static void PlaylistDequeue(ALVoice *voice, ALPlayBuf *node)
{
    //LOG("Dequeing OpenAL buffer %u", node->bufid);

    // remove from hash / place on list
    HASH_DEL(voice->queued_buffers, node);              // remove from hash
    node->_avail_next = voice->avail_buffers;
    voice->avail_buffers = node;

    voice->_queued_total_bytes -= node->bytes;
    assert(voice->_queued_total_bytes >= 0);
    node->bytes = 0;

#if 0
    ALPlayBuf *tmp = voice->queued_buffers;
    unsigned int count = HASH_COUNT(voice->queued_buffers);
    LOG("\t(numqueued: %d)", count);
    for (unsigned int i = 0; i < count; i++, tmp = tmp->hh.next)
    {
        LOG("\t(bufid : %u)", tmp->bufid);
    }
#endif
}

// ----------------------------------------------------------------------------

static long openal_systemSetup(INOUT AudioContext_s **audio_context)
{
    assert(*audio_context == NULL);
    assert(voices == NULL);

    ALCcontext *ctx = NULL;

    do {

        if ((ctx = InitAL()) == NULL)
        {
            ERRLOG("OOPS, OpenAL initialize failed");
            break;
        }

        if (alIsExtensionPresent("AL_SOFT_buffer_samples"))
        {
            LOG("AL_SOFT_buffer_samples supported, good!");
        }
        else
        {
            LOG("WARNING - AL_SOFT_buffer_samples extension not supported... Proceeding anyway...");
        }

        if ((*audio_context = malloc(sizeof(AudioContext_s))) == NULL)
        {
            ERRLOG("OOPS, Not enough memory");
            break;
        }

        (*audio_context)->_internal = ctx;
        (*audio_context)->CreateSoundBuffer = &OpenALCreateSoundBuffer;
        (*audio_context)->DestroySoundBuffer = &OpenALDestroySoundBuffer;

        return 0;
    } while(0);

    // ERRQUIT
    if (*audio_context)
    {
        FREE(*audio_context);
    }

    return -1;
}

static long openal_systemShutdown(INOUT AudioContext_s **audio_context)
{
    assert(*audio_context != NULL);

    ALCcontext *ctx = (ALCcontext*) (*audio_context)->_internal;
    assert(ctx != NULL);
    (*audio_context)->_internal = NULL;
    FREE(*audio_context);

    CloseAL();

    return 0;
}

// pause all audio
static long openal_systemPause(void)
{
    ALVoices *vnode = NULL;
    ALVoices *tmp = NULL;
    int err = 0;

    HASH_ITER(hh, voices, vnode, tmp) {
        alSourcePause(vnode->source);
        err = alGetError();
        if (err != AL_NO_ERROR)
        {
            ERRLOG("OOPS, Failed to pause source : 0x%08x", err);
        }
    }

    return 0;
}

static long openal_systemResume(void)
{
    ALVoices *vnode = NULL;
    ALVoices *tmp = NULL;
    int err = 0;

    HASH_ITER(hh, voices, vnode, tmp) {
        alSourcePlay(vnode->source);
        err = alGetError();
        if (err != AL_NO_ERROR)
        {
            ERRLOG("OOPS, Failed to pause source : 0x%08x", err);
        }
    }

    return 0;
}

// ----------------------------------------------------------------------------

/* Destroys a voice object, deleting the source and buffers. No error handling
 * since these calls shouldn't fail with a properly-made voice object. */
static void DeleteVoice(ALVoice *voice)
{
    alDeleteSources(1, &voice->source);
    if (alGetError() != AL_NO_ERROR)
    {
        ERRLOG("OOPS, Failed to delete source");
    }

    if (voice->data)
    {
        FREE(voice->data);
    }

    ALPlayBuf *node = NULL;
    ALPlayBuf *tmp = NULL;
    HASH_ITER(hh, voice->queued_buffers, node, tmp) {
        PlaylistDequeue(voice, node);
    }

    while (voice->avail_buffers)
    {
        node = voice->avail_buffers;
        alDeleteBuffers(1, &node->bufid);
        if (alGetError() != AL_NO_ERROR)
        {
            ERRLOG("OOPS, Failed to delete object IDs");
        }
        voice->avail_buffers = node->_avail_next;
        FREE(node);
    }

    memset(voice, 0, sizeof(*voice));
    FREE(voice);
}

/* Creates a new voice object, and allocates the needed OpenAL source and
 * buffer objects. Error checking is simplified for the purposes of this
 * example, and will cause an abort if needed. */
static ALVoice *NewVoice(const AudioParams_s *params)
{
    ALVoice *voice = NULL;

    do {
        voice = calloc(1, sizeof(*voice));
        if (voice == NULL)
        {
            ERRLOG("OOPS, Out of memory!");
            break;
        }

        ALuint buffers[OPENAL_NUM_BUFFERS];
        alGenBuffers(OPENAL_NUM_BUFFERS, buffers);
        if (alGetError() != AL_NO_ERROR)
        {
            ERRLOG("OOPS, Could not create buffers");
            break;
        }

        alGenSources(1, &voice->source);
        if (alGetError() != AL_NO_ERROR)
        {
            ERRLOG("OOPS, Could not create source");
            break;
        }

        // Set parameters so mono sources play out the front-center speaker and won't distance attenuate.
        alSource3i(voice->source, AL_POSITION, 0, 0, -1);
        if (alGetError() != AL_NO_ERROR)
        {
            ERRLOG("OOPS, Could not set AL_POSITION source parameter");
            break;
        }
        alSourcei(voice->source, AL_SOURCE_RELATIVE, AL_TRUE);
        if (alGetError() != AL_NO_ERROR)
        {
            ERRLOG("OOPS, Could not set AL_SOURCE_RELATIVE source parameter");
            break;
        }
        alSourcei(voice->source, AL_ROLLOFF_FACTOR, 0);
        if (alGetError() != AL_NO_ERROR)
        {
            ERRLOG("OOPS, Could not set AL_ROLLOFF_FACTOR source parameter");
            break;
        }

#if 0
        alSourcei(voice->source, AL_STREAMING, AL_TRUE);
        if (alGetError() != AL_NO_ERROR)
        {
            ERRLOG("OOPS, Could not set AL_STREAMING source parameter");
            break;
        }
#endif

        voice->avail_buffers = NULL;
        for (unsigned int i=0; i<OPENAL_NUM_BUFFERS; i++)
        {
            ALPlayBuf immutableNode = { .bufid = buffers[i] };
            ALPlayBuf *node = calloc(1, sizeof(ALPlayBuf));
            if (!node)
            {
                ERRLOG("OOPS, Not enough memory");
                break;
            }
            memcpy(node, &immutableNode, sizeof(ALPlayBuf));
            node->_avail_next = voice->avail_buffers;
            voice->avail_buffers = node;
        }

        voice->rate = (ALuint)params->nSamplesPerSec;

        // Emulator supports only mono and stereo 
        if (params->nChannels == 2)
        {
            voice->format = AL_FORMAT_STEREO16;
        }
        else
        {
            voice->format = AL_FORMAT_MONO16;
        }

        /* Allocate enough space for the temp buffer, given the format */
        voice->buffersize = (ALsizei)params->dwBufferBytes;
        voice->data = malloc(voice->buffersize);
        if (voice->data == NULL)
        {
            ERRLOG("OOPS, Error allocating %d bytes", voice->buffersize);
            break;
        }

        LOG("\tRate     : 0x%08x", voice->rate);
        LOG("\tFormat   : 0x%08x", voice->format);
        LOG("\tbuffersize : %d", voice->buffersize);

        return voice;

    } while(0);

    // ERR
    if (voice)
    {
        DeleteVoice(voice);
    }

    return NULL;
}

// ----------------------------------------------------------------------------

static long ALGetVolume(AudioBuffer_s *_this, long *volume)
{
    LOG("ALGetVolume ...");
    if (volume)
    {
        *volume = 0;
    }
    return 0;
}

static long ALSetVolume(AudioBuffer_s *_this, long volume)
{
    LOG("ALSetVolume ...");
    return 0;
}

static long ALStop(AudioBuffer_s *_this)
{
    LOG("ALStop ...");
    return 0;
}

static long ALRestore(AudioBuffer_s *_this)
{
    LOG("ALRestore ...");
    return 0;
}

static long ALPlay(AudioBuffer_s *_this, unsigned long reserved1, unsigned long reserved2, unsigned long flags)
{
    LOG("ALPlay ...");

    return 0;
}

static int _ALProcessPlayBuffers(ALVoice *voice, ALuint *bytes_queued)
{
    ALint processed = 0;
    int err = 0;
    *bytes_queued = 0;

    alGetSourcei(voice->source, AL_BUFFERS_PROCESSED, &processed);
    if ((err = alGetError()) != AL_NO_ERROR)
    {
        ERRLOG("OOPS, error in checking processed buffers : 0x%08x", err);
        return err;
    }

    if ((processed == 0) && (HASH_COUNT(voice->queued_buffers) >= OPENAL_NUM_BUFFERS))
    {
        //LOG("All audio buffers processing...");
    }

    while (processed > 0)
    {
        --processed;
        ALuint bufid = 0;
        alSourceUnqueueBuffers(voice->source, 1, &bufid);
        if ((err = alGetError()) != AL_NO_ERROR)
        {
            ERRLOG("OOPS, OpenAL error dequeuing buffer : 0x%08x", err);
            return err;
        }

        //LOG("Attempting to dequeue %u ...", bufid);
        ALPlayBuf *node = PlaylistGet(voice, bufid);
        if (!node)
        {
            ERRLOG("OOPS, OpenAL bufid %u not found in playlist...", bufid);
            ALPlayBuf *tmp = voice->queued_buffers;
            unsigned int count = HASH_COUNT(voice->queued_buffers);
            LOG("\t(numqueued: %d)", count);
            for (unsigned int i = 0; i < count; i++, tmp = tmp->hh.next)
            {
                LOG("\t(bufid : %u)", tmp->bufid);
            }
            continue;
        }

        PlaylistDequeue(voice, node);
    }

    ALint play_offset = 0;
    alGetSourcei(voice->source, AL_BYTE_OFFSET, &play_offset);
    if ((err = alGetError()) != AL_NO_ERROR)
    {
        ERRLOG("OOPS, alGetSourcei AL_BYTE_OFFSET : 0x%08x", err);
        return err;
    }
    assert((play_offset >= 0)/* && (play_offset < voice->buffersize)*/);

    long q = voice->_queued_total_bytes/* + voice->index*/ - play_offset;

    if (q >= 0) {
        *bytes_queued = (ALuint)q;
    }

    return 0;
}

// returns queued+working sound buffer size in bytes 
static long ALGetPosition(AudioBuffer_s *_this, unsigned long *bytes_queued, unsigned long *unused_write_cursor)
{
    ALVoice *voice = (ALVoice*)_this->_internal;
    *bytes_queued = 0;
    if (unused_write_cursor)
    {
        *unused_write_cursor = 0;
    }

    ALuint queued = 0;
    int err = _ALProcessPlayBuffers(voice, &queued);
    if (err)
    {
        return err;
    }
    static int last_queued = 0;
    if (queued != last_queued)
    {
        last_queued = queued;
        //LOG("OpenAL bytes queued : %u", queued);
    }

    *bytes_queued = queued + voice->index;

    return 0;
}

// DS->Lock()
static long ALBegin(AudioBuffer_s *_this, unsigned long unused, unsigned long write_bytes, INOUT int16_t **audio_ptr1, INOUT unsigned long *audio_bytes1, void **unused_audio_ptr2, unsigned long *unused_audio_bytes2, unsigned long flags_unused)
{
    ALVoice *voice = (ALVoice*)_this->_internal;

    if (unused_audio_ptr2)
    {
        *unused_audio_ptr2 = NULL;
    }

    if (write_bytes == 0)
    {
        write_bytes = voice->buffersize;
    }

    ALuint bytes_queued = 0;
    int err = _ALProcessPlayBuffers(voice, &bytes_queued);
    if (err)
    {
        return err;
    }

    if ((bytes_queued == 0) && (voice->index == 0))
    {
        LOG("Buffer underrun ... queuing quiet samples ...");
        int quiet_size = voice->buffersize>>2/* 1/4 buffer */;
        memset(voice->data, 0x0, quiet_size);
        voice->index += quiet_size;
    }
    else if (bytes_queued + voice->index < (voice->buffersize>>3)/* 1/8 buffer */)
    {
        LOG("Potential underrun ...");
    }

    ALsizei remaining = voice->buffersize - voice->index;
    if (write_bytes > remaining)
    {
        write_bytes = remaining;
    }

    *audio_ptr1 = voice->data+voice->index;
    *audio_bytes1 = write_bytes;

    return 0;
}

static int _ALSubmitBufferToOpenAL(ALVoice *voice)
{
    int err = 0;

    ALPlayBuf *node = PlaylistEnqueue(voice, voice->index);
    if (!node)
    {
        return -1;
    }
    //LOG("Enqueing OpenAL buffer %u (%u bytes)", node->bufid, node->bytes);
    alBufferData(node->bufid, voice->format, voice->data, node->bytes, voice->rate);

    if ((err = alGetError()) != AL_NO_ERROR)
    {
        PlaylistDequeue(voice, node);
        ERRLOG("OOPS, Error alBufferData : 0x%08x", err);
        return err;
    }

    alSourceQueueBuffers(voice->source, 1, &node->bufid);
    if ((err = alGetError()) != AL_NO_ERROR)
    {
        PlaylistDequeue(voice, node);
        ERRLOG("OOPS, Error buffering data : 0x%08x", err);
        return err;
    }

    ALint state = 0;
    alGetSourcei(voice->source, AL_SOURCE_STATE, &state);
    if ((err = alGetError()) != AL_NO_ERROR)
    {
        ERRLOG("OOPS, Error checking source state : 0x%08x", err);
        return err;
    }
    if ((state != AL_PLAYING) && (state != AL_PAUSED))
    {
        // 2013/11/17 NOTE : alSourcePlay() is expensive and causes audio artifacts, only invoke if needed
        LOG("Restarting playback (was 0x%08x) ...", state);
        alSourcePlay(voice->source);
        if ((err = alGetError()) != AL_NO_ERROR)
        {
            LOG("Error starting playback : 0x%08x", err);
            return err;
        }
    }

    return 0;
}

// DS->Unlock()
static long ALCommit(AudioBuffer_s *_this, void *unused_audio_ptr1, unsigned long audio_bytes1, void *unused_audio_ptr2, unsigned long unused_audio_bytes2)
{
    ALVoice *voice = (ALVoice*)_this->_internal;
    int err = 0;

    ALuint bytes_queued = 0;
    err = _ALProcessPlayBuffers(voice, &bytes_queued);
    if (err)
    {
        return err;
    }

    voice->index += audio_bytes1;

    while (voice->index > voice->buffersize)
    {
        // hopefully this is DEADC0DE or we've overwritten voice->data buffer ...
        ERRLOG("OOPS, overflow in queued sound data");
        assert(false);
    }

    if (bytes_queued >= (voice->buffersize>>2)/*quarter buffersize*/)
    {
        // keep accumulating data into working buffer
        return 0;
    }

    if (HASH_COUNT(voice->queued_buffers) >= (OPENAL_NUM_BUFFERS))
    {
        //LOG("no free audio buffers"); // keep accumulating ...
        return 0;
    }

    // ---------------------------
    // Submit working buffer to OpenAL

    err = _ALSubmitBufferToOpenAL(voice);
    if (err)
    {
        return err;
    }

    return 0;
}

// HACK Part I : done once for mockingboard that has semiauto repeating phonemes ...
static long ALCommitStaticBuffer(AudioBuffer_s *_this, unsigned long audio_bytes1)
{
    ALVoice *voice = (ALVoice*)_this->_internal;
    voice->replay_index = (ALsizei)audio_bytes1;
    return 0;
}

// HACK Part II : replay mockingboard phoneme ...
static long ALReplay(AudioBuffer_s *_this)
{
    ALVoice *voice = (ALVoice*)_this->_internal;
    voice->index = voice->replay_index;

    int err = 0;

    err = _ALSubmitBufferToOpenAL(voice);
    if (err)
    {
        return err;
    }

    return 0;
}

static long ALGetStatus(AudioBuffer_s *_this, unsigned long *status)
{
    ALVoice* voice = (ALVoice*)_this->_internal;

    int err = 0;
    ALint state = 0;
    alGetSourcei(voice->source, AL_SOURCE_STATE, &state);
    if ((err = alGetError()) != AL_NO_ERROR)
    {
        ERRLOG("OOPS, Error checking source state : 0x%08x", err);
        return err;
    }

    if ((state == AL_PLAYING) || (state == AL_PAUSED))
    {
        *status = AUDIO_STATUS_PLAYING;
    }
    else
    {
        *status = AUDIO_STATUS_NOTPLAYING;
    }

    return 0;
}

static long OpenALCreateSoundBuffer(const AudioParams_s *params, INOUT AudioBuffer_s **soundbuf_struct, const AudioContext_s *audio_context)
{
    LOG("OpenALCreateSoundBuffer ...");
    assert(*soundbuf_struct == NULL);

    ALCcontext *ctx = (ALCcontext*)(audio_context->_internal);
    assert(ctx != NULL);

    ALVoice *voice = NULL;

    do {

        if ((voice = NewVoice(params)) == NULL)
        {
            ERRLOG("OOPS, Cannot create new voice");
            break;
        }

        ALVoices immutableNode = { .source = voice->source };
        ALVoices *vnode = calloc(1, sizeof(ALVoices));
        if (!vnode)
        {
            ERRLOG("OOPS, Not enough memory");
            break;
        }
        memcpy(vnode, &immutableNode, sizeof(ALVoices));
        vnode->voice = voice;
        HASH_ADD_INT(voices, source, vnode);

        if ((*soundbuf_struct = malloc(sizeof(AudioBuffer_s))) == NULL)
        {
            ERRLOG("OOPS, Not enough memory");
            break;
        }

        (*soundbuf_struct)->_internal          = voice;
        (*soundbuf_struct)->SetVolume          = &ALSetVolume;
        (*soundbuf_struct)->GetVolume          = &ALGetVolume;
        (*soundbuf_struct)->GetCurrentPosition = &ALGetPosition;
        (*soundbuf_struct)->Stop               = &ALStop;
        (*soundbuf_struct)->Restore            = &ALRestore;
        (*soundbuf_struct)->Play               = &ALPlay;
        (*soundbuf_struct)->Lock               = &ALBegin;
        (*soundbuf_struct)->Unlock             = &ALCommit;
        (*soundbuf_struct)->GetStatus          = &ALGetStatus;
        // mockingboard-specific hacks
        (*soundbuf_struct)->UnlockStaticBuffer = &ALCommitStaticBuffer;
        (*soundbuf_struct)->Replay             = &ALReplay;

        return 0;
    } while(0);

    if (*soundbuf_struct)
    {
        OpenALDestroySoundBuffer(soundbuf_struct);
    }
    else if (voice)
    {
        DeleteVoice(voice);
    }

    return -1;
}

static long OpenALDestroySoundBuffer(INOUT AudioBuffer_s **soundbuf_struct)
{
    if (!*soundbuf_struct) {
        // already dealloced
        return 0;
    }
    LOG("OpenALDestroySoundBuffer ...");
    ALVoice *voice = (ALVoice *)((*soundbuf_struct)->_internal);
    ALint source = voice->source;

    DeleteVoice(voice);

    ALVoices *vnode = NULL;
    HASH_FIND_INT(voices, &source, vnode);
    if (vnode) {
        HASH_DEL(voices, vnode);
        FREE(vnode);
    }

    FREE(*soundbuf_struct);
    return 0;
}

__attribute__((constructor(CTOR_PRIORITY_EARLY)))
static void _init_openal(void) {
    LOG("Initializing OpenAL sound system");

    assert(audio_backend == NULL && "there can only be one!");

    openal_audio_backend.setup            = &openal_systemSetup;
    openal_audio_backend.shutdown         = &openal_systemShutdown;
    openal_audio_backend.pause            = &openal_systemPause;
    openal_audio_backend.resume           = &openal_systemResume;

    audio_backend = &openal_audio_backend;
}

