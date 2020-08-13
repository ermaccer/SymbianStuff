#include <e32base.h>
#include <MdaAudioOutputStream.h>
#include <mda/common/audio.h>

_LIT(KQ3AAudioThreadName, "Q3AAudioThread");
static const TInt Q3QAudioThreadStackSize = 32768;

TBool audioStarted = EFalse;
RThread audioThread;
RMutex  audioMutex;

extern "C" void AudioStreamCallback(unsigned char*& bufaddr, unsigned int bufsize);





class CAudioStream : public CBase, public MMdaAudioOutputStreamCallback
    { 
    public:
        static CAudioStream* NewL(TInt aSampleRate, TInt aChannels, TInt aPreferredBufSize);
        virtual ~CAudioStream(); 
        void MaoscOpenComplete(TInt aError);
        void MaoscBufferCopied(TInt aError, const TDesC8 &aBuffer);
        void MaoscPlayComplete(TInt aError);
        void DoWriteBuffer();

    protected:
        CAudioStream(TInt aSampleRate, TInt aChannels, TInt aPreferredBufSize);
        void ConstructL();
    private:
        CMdaAudioOutputStream* iAudioStream;
        TMdaAudioDataSettings iAudioSettings;
        TInt iSampleRate;
        TInt iChannels;
        TInt iPreferredBufSize;
        TPtrC8 iWriteDes;
        TInt iBytesWritten;
    };



class CAudioTimer : public CActive
    {
    public:
        static CAudioTimer* NewL(CAudioStream* aStream);
        virtual ~CAudioTimer();
        void DoCancel();
        void RunL();
        void IssueRequest(TInt aTimeToWait);
    protected:
        CAudioTimer(CAudioStream* aStream);
        void ConstructL();
    private:
        RTimer iTimer;
        CAudioStream* iAudioStream;
    };
    
CAudioTimer* CAudioTimer::NewL(CAudioStream* aStream)
    {
    CAudioTimer* self = new (ELeave) CAudioTimer(aStream);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
    return self;
    }
    
CAudioTimer::CAudioTimer(CAudioStream* aStream) : CActive(EPriorityStandard), iAudioStream(aStream)
    {
        
    }
    
CAudioTimer::~CAudioTimer()
    {
    Cancel();
    iTimer.Close();
    }
    
void CAudioTimer::DoCancel()
    {
    iTimer.Cancel();    
    }
    
void CAudioTimer::ConstructL()
    {
    iTimer.CreateLocal();
    CActiveScheduler::Add(this);
    }
    
void CAudioTimer::IssueRequest(TInt aTimeToWait)
    {
    TTimeIntervalMicroSeconds32 microseconds(aTimeToWait);
    iTimer.After(iStatus, aTimeToWait);
    SetActive();
    }
    
void CAudioTimer::RunL()
    {
    iAudioStream->DoWriteBuffer();
    }
    
CAudioTimer* audioTimer = NULL;

CAudioStream::CAudioStream(TInt aSampleRate, TInt aChannels, TInt aPreferredBufSize) :
        iSampleRate(aSampleRate), iChannels(aChannels), iPreferredBufSize(aPreferredBufSize)
    {
    
    }
    
CAudioStream::~CAudioStream()
    {
    
   
    }
      
void CAudioStream::ConstructL()
    {
    iAudioStream = CMdaAudioOutputStream::NewL(*this);
    iAudioStream->Open(&iAudioSettings);
    }
    
CAudioStream* CAudioStream::NewL(TInt aSampleRate, TInt aChannels, TInt aPreferredBufSize)
    {
    CAudioStream* self = new (ELeave) CAudioStream(aSampleRate, aChannels, aPreferredBufSize);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
    return self;
    }

void CAudioStream::MaoscOpenComplete(TInt aError)
    {
    TInt samplerate;
    TInt channels;
    switch(iSampleRate)
        {
        case 11025:
            samplerate = TMdaAudioDataSettings::ESampleRate11025Hz;
            break;
        case 22050:
            samplerate = TMdaAudioDataSettings::ESampleRate22050Hz;
            break;
        case 44100:
            samplerate = TMdaAudioDataSettings::ESampleRate44100Hz;
            break;
        case 48000:
            samplerate = TMdaAudioDataSettings::ESampleRate48000Hz;
            break;
        default:        
            samplerate = TMdaAudioDataSettings::ESampleRate11025Hz;
            break;
        }
    switch (iChannels)
        {
        case 1:
            channels = TMdaAudioDataSettings::EChannelsMono;
            break;
        case 2:
        default:
            channels = TMdaAudioDataSettings::EChannelsStereo;
            break;
        }
    iAudioStream->SetAudioPropertiesL(samplerate, channels);
    iAudioStream->SetVolume(iAudioStream->MaxVolume());
    iAudioStream->SetPriority(EMdaPriorityMax, EMdaPriorityPreferenceTimeAndQuality);
    DoWriteBuffer();
    }
    

void CAudioStream::MaoscBufferCopied(TInt aError, const TDesC8 &aBuffer)
    {
    DoWriteBuffer(); 
    }

void CAudioStream::MaoscPlayComplete(TInt aError)
    {
    
    }
    
void CAudioStream::DoWriteBuffer()
    {
    unsigned char* buffer = NULL;
    TInt bytesrendered = iAudioStream->GetBytes();
    TInt buffered = iBytesWritten-bytesrendered;
    if (buffered > 8192)
        {
//        buffered-4096
        
  //      ms = (((buffered-4096)/2)*1000*1000)/11050;
        audioTimer->IssueRequest((((buffered-8192)/2)*1000*1000)/22050);
        return;
        }
    
    audioMutex.Wait();
    AudioStreamCallback(buffer, iPreferredBufSize);
    audioMutex.Signal();
    iWriteDes.Set(buffer, iPreferredBufSize);
    TRAP_IGNORE(iAudioStream->WriteL(iWriteDes));
    iBytesWritten+=iPreferredBufSize;
    }
    

TInt Q3AAudioThreadFunc(TAny* aPtr)
    {
    CTrapCleanup* cleanup = CTrapCleanup::New();
    CActiveScheduler* scheduler = new CActiveScheduler;
    if (scheduler)
        {
        CActiveScheduler::Install(scheduler);
        CAudioStream* audioStream = NULL;
        TRAP_IGNORE(audioStream = CAudioStream::NewL(22050, 2, 1024));
        TRAP_IGNORE(audioTimer = CAudioTimer::NewL(audioStream));
        CActiveScheduler::Start();
        delete audioStream;
        CActiveScheduler::Install(NULL);
        }
    delete scheduler;
    delete cleanup;
    return KErrNone;
    }

void AudioStream_Lockbuffer()
    {
    audioMutex.Wait();
    }
    
void AudioStream_UnlockBuffer()
    {
    audioMutex.Signal();
    }
TInt AudioStream_Start(void)
    {
    if (audioStarted)
        {
        return KErrAlreadyExists;
        }
    audioMutex.CreateLocal();
    audioThread.Create(KQ3AAudioThreadName, Q3AAudioThreadFunc, Q3QAudioThreadStackSize,NULL,NULL);
    audioThread.SetPriority(EPriorityMore);
    audioThread.Resume();
    return KErrNone;    
    }


