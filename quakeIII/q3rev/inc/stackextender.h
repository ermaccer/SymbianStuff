#ifndef __STACKEXTENDER__H__
#define __STACKEXTENDER__H__

#include <e32std.h>
#include <e32base.h>

class StackExtender
    {
    public:
        static void ExtendAndReplaceStack(TUint32 aNewStackSize);
        static void DestroyExtendedStack();
    };

#endif

