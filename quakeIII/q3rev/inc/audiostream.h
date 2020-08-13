#ifndef __AUDIOSTREAM__H__
#define __AUDIOSTREAM__H__

#include <MdaAudioOutputStream.h>
#include <mda/common/audio.h>

struct TAudioSubmissionChunk
    {
    public:
        TUint8* iAddress;
        TInt iChunkSize;
    };

class CAudioOutputStream : public CBase, public MMdaAudioOutputStreamCallback
    {
    public:
        static CAudioOutputStream* NewL(TInt aSamplerate, TInt aChannels);
        virtual ~CAudioOutputStream();
        void MaoscOpenComplete(TInt aError);
        void MaoscBufferCopied(TInt aError, const TDesC8 &aBuffer);
        void MaoscPlayComplete(TInt aError);
        void Stop();
        TInt Write(TAudioSubmissionChunk& aSubmissionChunk);
        TInt BlocksCopied();
    
    protected:
        CAudioOutputStream(TInt aSamplerate, TInt aChannels);
        void ConstructL();
    private:
        enum TAudioStreamState
            {
            EStreamStateNotReady    = 0,
            EStreamStateStopped     = 1,
            EStreamStateCopying     = 2,
            EStreamStateReady       = 4
            };

        TInt iSampleRate;
        TInt iChannels;
        TMdaAudioDataSettings iAudioSettings;
        CMdaAudioOutputStream* iAudioOutputStream;
        RArray<TAudioSubmissionChunk> iSubmissionChunks;
        RSemaphore iInitWait;
        TAudioStreamState iStreamState;
        TPtrC8 iWriteDes;
        TInt iBlocksCopied;  
        TBool iIsStopped;        
    };
    
#endif