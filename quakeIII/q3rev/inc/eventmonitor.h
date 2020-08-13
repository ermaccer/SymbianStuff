#ifndef __EVENTMONITOR_H__
#define __EVENTMONITOR_H__

#include <e32base.h>
#include <w32std.h>

void HandleWsEvent( const TWsEvent& aWsEvent );

class CQuakeEventMonitor : public CActive
    {
    public:
        virtual void RunL();
        virtual void DoCancel();
        static CQuakeEventMonitor* NewL();
        ~CQuakeEventMonitor();

    protected: 
        CQuakeEventMonitor();
        void ConstructL( );                

    private: 
        RWsSession iWsSession; 
    };

#endif
