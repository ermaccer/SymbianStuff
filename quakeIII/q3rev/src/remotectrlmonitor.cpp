#include "../ui/keycodes.h"
#include "../client/client.h"
#include "symbian_local.h"

#include "remotectrlmonitor.h"


CRemConEventTimer* CRemConEventTimer::NewL(TInt aKeyCode) 
    {
    CRemConEventTimer* self = new (ELeave) CRemConEventTimer(aKeyCode);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
    return self;
    }
CRemConEventTimer::~CRemConEventTimer() 
    {
    Cancel();
    iTimer.Close();
    }
    
void CRemConEventTimer::DoCancel() 
    {
    iTimer.Cancel();
    }
    
void CRemConEventTimer::RunL() 
    {
    if (iStatus.Int() == KErrCancel)
        {
        return;
        } 
    Sys_QueEvent( 0, SE_KEY, iKeyCode, qfalse, 0, NULL );
    }
    
void CRemConEventTimer::IssueRequest(TInt aTimeOutInMillis) 
    {
    iTimer.After(iStatus, aTimeOutInMillis*1000);
    SetActive();
    }

CRemConEventTimer::CRemConEventTimer(TInt aKeyCode) : CActive(EPriorityStandard), iKeyCode(aKeyCode) 
    {
    CActiveScheduler::Add(this);
    }

void CRemConEventTimer::ConstructL() 
    {
    User::LeaveIfError(iTimer.CreateLocal());
    }



int CRemoteCtrlEventMonitor::MapRemoteCtrlKey(TRemConCoreApiOperationId aOp)
    {
    switch (aOp)
        {
        case ERemConCoreApiVolumeUp:
            return K_PGUP;
        case ERemConCoreApiVolumeDown:                    
            return K_PGDN;
        default: return 0;
        }
    }


CRemoteCtrlEventMonitor::CRemoteCtrlEventMonitor() 
    {
    }
    
CRemoteCtrlEventMonitor::~CRemoteCtrlEventMonitor()
    {
    delete iInterfaceSelector;    
    delete iCancelTimers[0];
    delete iCancelTimers[1];
    }

CRemoteCtrlEventMonitor* CRemoteCtrlEventMonitor::NewL()
    {
    CRemoteCtrlEventMonitor* self = new (ELeave)CRemoteCtrlEventMonitor;
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
    return self;
    }

void CRemoteCtrlEventMonitor::ConstructL()
    {
    iInterfaceSelector = CRemConInterfaceSelector::NewL();
    iCoreTarget = CRemConCoreApiTarget::NewL(*iInterfaceSelector, *this);
    iInterfaceSelector->OpenTargetL();
    iCancelTimers[0] = CRemConEventTimer::NewL(K_PGUP);
    iCancelTimers[1] = CRemConEventTimer::NewL(K_PGDN);
    }
    
    
void CRemoteCtrlEventMonitor::MrccatoCommand(TRemConCoreApiOperationId aOperationId, TRemConCoreApiButtonAction aButtonAct)
    {
    TRequestStatus status;
    int remconcode = MapRemoteCtrlKey(aOperationId);
    switch (aButtonAct)
        {
        case ERemConCoreApiButtonPress:
            if (remconcode == K_PGUP)
                {
                iCancelTimers[0]->Cancel();
                }
            else if (remconcode == K_PGDN)
                {
                iCancelTimers[1]->Cancel();
                }
            break;
        case ERemConCoreApiButtonRelease:
            Sys_QueEvent( 0, SE_KEY, remconcode, qfalse, 0, NULL );
            break;
        case ERemConCoreApiButtonClick:
            if (remconcode == K_PGUP)
                {          
                if (iCancelTimers[0]->IsActive())
                    {
                    Sys_QueEvent( 0, SE_KEY, remconcode, qfalse, 0, NULL );
                    iCancelTimers[0]->Cancel();
                    }
                iCancelTimers[0]->IssueRequest(KAutoCancelTimeOutInMillis);
                }
            else if (remconcode == K_PGDN)
                {
                if (iCancelTimers[1]->IsActive())
                    {
                    Sys_QueEvent( 0, SE_KEY, remconcode, qfalse, 0, NULL );
                    iCancelTimers[1]->Cancel();
                    }
                iCancelTimers[1]->IssueRequest(KAutoCancelTimeOutInMillis);
                }
            Sys_QueEvent( 0, SE_KEY, remconcode, qtrue, 0, NULL );
            break;
        default:
            break;
        }
    iCoreTarget->SendResponse(status, aOperationId, KErrNone);
    User::WaitForRequest(status);        
    }

