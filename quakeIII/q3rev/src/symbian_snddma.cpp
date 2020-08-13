/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// snddma_null.c
// all other sound mixing is portable

#include "../client/client.h"
#include "../client/snd_local.h"
#include "audiostream.h"

#define	WAV_BUFFERS				16
#define	WAV_MASK				15//0x3F
#define	WAV_BUFFER_SIZE			512 //4096
EXTERN_C int Symbian_ScheduleEvents(int aPriority);
CAudioOutputStream* audioStream;
int	sample16;
int	snd_sent, snd_completed;
unsigned char* snd_buffer;

static int dmapos = 0;
TInt AudioStream_Start(void);
void AudioStream_Lockbuffer(void);    
void AudioStream_UnlockBuffer(void);

EXTERN_C void AudioStreamCallback(unsigned char*& bufaddr, unsigned int bufsize);

void AudioStreamCallback(unsigned char*& bufaddr, unsigned int bufsize)
    {
    bufaddr = snd_buffer+dmapos;
    dmapos+=bufsize;
    if (dmapos >= 65536*2)
        {
        dmapos = 0;
        }
    
    }


qboolean SNDDMA_Init(void)
    {
    snd_buffer = (unsigned char*)malloc(65536*2);
    memset(snd_buffer, 0, 65536*2);
        
	snd_sent = 0;
	snd_completed = 0;

	dma.channels = 2;
	dma.samplebits = 16;
	dma.speed = 22050;
	dma.samples = 65536;
	dma.submission_chunk = 512;
	dma.buffer = (unsigned char *) snd_buffer;
	sample16 = (dma.samplebits/8) - 1;

    AudioStream_Start();
	Com_Printf ("*** SNDDMA initialized ***\n");
    return qtrue;
    }
    
int submitcount = 0;
int offset = 0;


int SNDDMA_GetDMAPos(void)
    {
    return dmapos/2;
    }
    
    
void SNDDMA_Submit(void)
    {
    void AudioStream_UnlockBuffer(void);
    }


void SNDDMA_Shutdown(void)
    {
    free(snd_buffer);   
    }

void SNDDMA_BeginPainting (void)
    {
    void AudioStream_LockBuffer(void);
    }

