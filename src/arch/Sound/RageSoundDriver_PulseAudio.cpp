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

/* Buffer size in frames - we'll write this much audio at a time */
const int chunksize = 1024;

int RageSound_PulseAudio::MixerThread_start( void *p )
{
	((RageSound_PulseAudio *) p)->MixerThread();
	return 0;
}

void RageSound_PulseAudio::MixerThread()
{
	/* Increase the priority of this thread to reduce latency */
	SetupDecodingThread();

	while( !m_bShutdown )
	{
		/* Get current hardware position */
		int64_t current_pos = m_iWritePos;

		/* Allocate buffer for mixing */
		int16_t buf[chunksize * channels];

		/* Mix audio from all playing sounds */
		this->Mix( buf, chunksize, m_iWritePos, current_pos );

		/* Write to PulseAudio */
		pa_simple *s = (pa_simple*)m_pPulseAudio;
		int error;

		if( pa_simple_write( s, buf, chunksize * bytes_per_frame, &error ) < 0 )
		{
			LOG->Warn( "PulseAudio write failed: %s", pa_strerror(error) );
			/* Don't spam warnings - sleep a bit and continue */
			usleep( 10000 );
			continue;
		}

		/* Update write position */
		m_iWritePos += chunksize;

		/* Sleep for a bit to avoid consuming too much CPU.
		 * We sleep for about half the chunk duration to keep the buffer filled. */
		usleep( (chunksize * 500000) / samplerate );
	}
}

void RageSound_PulseAudio::SetupDecodingThread()
{
	/* Set this thread to a higher priority */
	/* This is optional - implement if needed for better latency */
}

RageSound_PulseAudio::RageSound_PulseAudio()
{
	m_bShutdown = false;
	m_iWritePos = 0;
	m_pPulseAudio = nullptr;

	/* Set up PulseAudio sample specification */
	pa_sample_spec ss;
	ss.format = PA_SAMPLE_S16LE;  /* Signed 16-bit little-endian */
	ss.channels = channels;
	ss.rate = samplerate;

	/* Set up buffer attributes for low latency */
	pa_buffer_attr ba;
	ba.maxlength = (uint32_t) -1;  /* Maximum length of buffer */
	ba.tlength = chunksize * bytes_per_frame * 4;  /* Target buffer length */
	ba.prebuf = (uint32_t) -1;  /* Prebuffer amount */
	ba.minreq = (uint32_t) -1;  /* Minimum request */
	ba.fragsize = (uint32_t) -1;  /* Fragment size (for recording) */

	int error;

	/* Create a simple playback stream */
	pa_simple *s = pa_simple_new(
		nullptr,           /* Use default server */
		"StepMania",      /* Application name */
		PA_STREAM_PLAYBACK,  /* Playback stream */
		nullptr,           /* Use default device */
		"Game Audio",     /* Stream description */
		&ss,              /* Sample format */
		nullptr,           /* Use default channel map */
		&ba,              /* Buffering attributes */
		&error            /* Error code */
	);

	if( !s )
	{
		RageException::Throw( "PulseAudio initialization failed: %s", pa_strerror(error) );
	}

	m_pPulseAudio = s;

	/* Calculate latency based on buffer size */
	m_fLatency = (float)(chunksize * 4) / (float)samplerate;

	LOG->Info( "PulseAudio driver initialized: %d Hz, %d channels, %.3f second latency",
		samplerate, channels, m_fLatency );

	/* Start the decoding thread (from base class) */
	StartDecodeThread();

	/* Start the mixing thread */
	m_MixingThread.SetName( "PulseAudio mixer" );
	m_MixingThread.Create( MixerThread_start, this );
}

RageSound_PulseAudio::~RageSound_PulseAudio()
{
	/* Signal shutdown */
	m_bShutdown = true;

	/* Wait for mixing thread to finish */
	LOG->Trace( "Shutting down PulseAudio mixer thread..." );
	m_MixingThread.Wait();
	LOG->Trace( "PulseAudio mixer thread stopped." );

	/* Drain and close PulseAudio */
	if( m_pPulseAudio )
	{
		pa_simple *s = (pa_simple*)m_pPulseAudio;
		int error;

		/* Drain the stream - wait for all audio to finish playing */
		if( pa_simple_drain( s, &error ) < 0 )
		{
			LOG->Warn( "PulseAudio drain failed: %s", pa_strerror(error) );
		}

		/* Free the stream */
		pa_simple_free( s );
		m_pPulseAudio = nullptr;
	}
}

int64_t RageSound_PulseAudio::GetPosition( const RageSoundBase *snd ) const
{
	/* Return the current write position minus the latency buffer */
	pa_simple *s = (pa_simple*)m_pPulseAudio;
	if( !s )
		return m_iWritePos;

	int error;
	pa_usec_t latency_usec = pa_simple_get_latency( s, &error );

	if( latency_usec == (pa_usec_t) -1 )
	{
		/* Error getting latency, just return write position */
		return m_iWritePos;
	}

	/* Convert latency from microseconds to frames */
	int64_t latency_frames = (int64_t)((latency_usec * samplerate) / 1000000);

	/* Return write position minus latency */
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
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
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
