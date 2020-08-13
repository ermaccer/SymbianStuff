#include <e32std.h>
#include <e32cmn.h>

#include "stackextender.h"

extern "C" TUint32 __StackExtender_ReadSP();
extern "C" void __StackExtender_WriteSP(TUint32 aNewValue);

TUint8* theOldStack = 0;
TUint8* theNewStack = 0;
TUint32 oldStackSize = 0;
TUint32 newStackSize = 0;

void StackExtender::ExtendAndReplaceStack(TUint32 aNewStackSize)
    {
#if !defined(__WINS__)
    RThread myself;
    TThreadStackInfo theStackInfo;
    
    theNewStack = new TUint8[aNewStackSize];
    newStackSize = aNewStackSize;
    myself.StackInfo(theStackInfo);// iBase, iLimit
    theOldStack = (TUint8*)theStackInfo.iLimit;
    oldStackSize = theStackInfo.iBase-theStackInfo.iLimit;
    
    // copy the entire old stack to new one...
    Mem::FillZ(theNewStack,aNewStackSize);
    Mem::Copy(theNewStack+aNewStackSize-oldStackSize,(TUint8*)theStackInfo.iLimit, oldStackSize);
    TUint32 theSP = (TUint32)__StackExtender_ReadSP();
    TUint32 offset = theStackInfo.iBase-theSP;
    __StackExtender_WriteSP((TUint32)(theNewStack+aNewStackSize-offset));               
#endif
    }
    
void StackExtender::DestroyExtendedStack()
    {
#if !defined(__WINS__)
    // fill the old stack from the new stack...
    Mem::Copy(theOldStack,theNewStack+newStackSize-oldStackSize,oldStackSize);
    TUint32 theSP = (TUint32)__StackExtender_ReadSP();
    TUint32 offset = ((TUint32)(theNewStack))+newStackSize-theSP;
    __StackExtender_WriteSP((TUint32)(theOldStack+oldStackSize-offset));
    delete [] theNewStack;
    theOldStack = 0;
    theNewStack = 0;
    oldStackSize = 0;
    newStackSize = 0;
#endif
    }

