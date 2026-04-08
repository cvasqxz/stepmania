#include "global.h"
#include "RageSoundDriver_PulseAudio.h"

#include "RageLog.h"
#include "RageSound.h"
#include "RageUtil.h"
#include "RageTimer.h"

#include <pulse/simple.h>
#include <pulse/error.h>

const int channels = 2;
const int samplerate = 44100;
const int bytes_per_frame = channels * sizeof(int16_t);

/* Write this many frames per chunk. ~23ms at 44100Hz. */
const int chunksize = 1024;

int RageSound_PulseAudio::MixerThread_start( void *p )
{
	((RageSound_PulseAudio *) p)->MixerThread();
	return 0;
}

void RageSound_PulseAudio::MixerThread()
{
	SetupDecodingThread();

	int16_t buf[chunksize * channels];

	while( !m_bShutdown )
	{
		int64_t play_pos = GetPosition( NULL );

		this->Mix( buf, chunksize, m_iWritePos, play_pos );

		pa_simple *s = (pa_simple*)m_pPulseAudio;
		int error;
		if( pa_simple_write( s, buf, chunksize * bytes_per_frame, &error ) < 0 )
		{
			LOG->Warn( "PulseAudio write failed: %s", pa_strerror(error) );
			usleep( 10000 );
			continue;
		}

		m_iWritePos += chunksize;
	}
}

void RageSound_PulseAudio::SetupDecodingThread()
{
	/* No-op: PulseAudio's simple API handles timing internally. */
}

RageSound_PulseAudio::RageSound_PulseAudio()
{
	m_bShutdown = false;
	m_iWritePos = 0;
	m_pPulseAudio = nullptr;

	pa_sample_spec ss;
	ss.format = PA_SAMPLE_S16LE;
	ss.channels = channels;
	ss.rate = samplerate;

	/* Target ~4 chunks of buffering for smooth playback. */
	pa_buffer_attr ba;
	ba.maxlength = (uint32_t) -1;
	ba.tlength   = chunksize * bytes_per_frame * 4;
	ba.prebuf    = (uint32_t) -1;
	ba.minreq    = (uint32_t) -1;
	ba.fragsize  = (uint32_t) -1;

	int error;
	pa_simple *s = pa_simple_new(
		nullptr,              /* default server */
		"StepMania",
		PA_STREAM_PLAYBACK,
		nullptr,              /* default device — follows system default */
		"Game Audio",
		&ss,
		nullptr,              /* default channel map */
		&ba,
		&error
	);

	if( !s )
		RageException::Throw( "PulseAudio initialization failed: %s", pa_strerror(error) );

	m_pPulseAudio = s;

	m_fLatency = (float)(chunksize * 4) / (float)samplerate;

	LOG->Info( "PulseAudio driver initialized: %d Hz, %d ch, %.3f s latency",
		samplerate, channels, m_fLatency );

	StartDecodeThread();

	m_MixingThread.SetName( "PulseAudio mixer" );
	m_MixingThread.Create( MixerThread_start, this );
}

RageSound_PulseAudio::~RageSound_PulseAudio()
{
	m_bShutdown = true;

	LOG->Trace( "Shutting down PulseAudio mixer thread..." );
	m_MixingThread.Wait();
	LOG->Trace( "PulseAudio mixer thread stopped." );

	if( m_pPulseAudio )
	{
		pa_simple *s = (pa_simple*)m_pPulseAudio;
		int error;
		if( pa_simple_drain( s, &error ) < 0 )
			LOG->Warn( "PulseAudio drain failed: %s", pa_strerror(error) );
		pa_simple_free( s );
		m_pPulseAudio = nullptr;
	}
}

int64_t RageSound_PulseAudio::GetPosition( const RageSoundBase *snd ) const
{
	pa_simple *s = (pa_simple*)m_pPulseAudio;
	if( !s )
		return m_iWritePos;

	int error;
	pa_usec_t latency_usec = pa_simple_get_latency( s, &error );
	if( latency_usec == (pa_usec_t) -1 )
		return m_iWritePos;

	int64_t latency_frames = (int64_t)((latency_usec * samplerate) / 1000000ULL);
	return m_iWritePos - latency_frames;
}

float RageSound_PulseAudio::GetPlayLatency() const
{
	return m_fLatency;
}

/*
 * (c) 2026 StepMania Development Team
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, subject to the above
 * copyright notice(s) and this permission notice appearing in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appearing in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
