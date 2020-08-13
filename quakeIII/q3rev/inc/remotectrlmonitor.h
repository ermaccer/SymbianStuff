#ifndef __REMOTECTRLMONITOR_H__
#define __REMOTECTRLMONITOR_H__

#include <e32std.h>
#include <e32base.h>
#include <remconcoreapitargetobserver.h>
#include <remconinterfaceselector.h>
#include <remconcoreapitarget.h>

static const TInt KAutoCancelTimeOutInMillis = 650;

class CRemConEventTimer : public CActive
    {
    public:
        static CRemConEventTimer* NewL(TInt aKeyCode);
        virtual ~CRemConEventTimer();
        void DoCancel();
        void RunL();
        void IssueRequest(TInt aTimeOutInMillis);
    protected:
        CRemConEventTimer(TInt aKeyCode);
        void ConstructL();
    private:
        RTimer iTimer;
        TInt iKeyCode;
    };


class CRemoteCtrlEventMonitor : public CBase, public MRemConCoreApiTargetObserver
    {
    public:
        static CRemoteCtrlEventMonitor* NewL();
        ~CRemoteCtrlEventMonitor();
        void MrccatoCommand(TRemConCoreApiOperationId aOperationId,
                            TRemConCoreApiButtonAction aButtonAct);
    protected:
        CRemoteCtrlEventMonitor();
        void ConstructL();
        int MapRemoteCtrlKey(TRemConCoreApiOperationId aOp);
    private:
        CRemConInterfaceSelector* iInterfaceSelector;
        CRemConCoreApiTarget*     iCoreTarget;
        CRemConEventTimer* iCancelTimers[2];
    };

#endif
