#ifndef RAGE_SOUND_DRIVER_PULSEAUDIO_H
#define RAGE_SOUND_DRIVER_PULSEAUDIO_H

#include "RageSoundDriver_Generic_Software.h"

class RageSound_PulseAudio: public RageSound_Generic_Software
{
public:
	RageSound_PulseAudio();
	~RageSound_PulseAudio();

	int64_t GetPosition( const RageSoundBase *snd ) const;
	float GetPlayLatency() const;
	int GetSampleRate( int rate ) const { return 44100; }

private:
	static int MixerThread_start( void *p );
	void MixerThread();
	void SetupDecodingThread();

	RageThread m_MixingThread;
	bool m_bShutdown;
	int64_t m_iWritePos;

	void *m_pPulseAudio;      // pa_simple* (opaque pointer)
	float m_fLatency;
};

#endif

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
