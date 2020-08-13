#ifndef __HUNKMEM_H__
#define __HUNKMEM_H__

#include "cw_win32off.h"
#include <e32base.h>
#include <e32cmn.h>

#define ALIGN_TO_PAGESIZE(x) {x=(x+4095)&~4095;}
#define ROUND_TO_PAGESIZE(x) {x=(x)&~4095;}



class CHunkMem : public CBase
    {
    public:
        static CHunkMem* NewL(TInt aSize);
        virtual ~CHunkMem();
        
        TAny* Alloc(TInt aSize);
        TAny* AllocTemporary(TInt aSize);
        TInt FreeTemporary(TAny* aPtr, TInt aSize);
        void ClearTemporary();
        void SwapBanks();
        void Clear();
        void SetMark();
        void ClearToMark();
    private:
        void ConstructL();
        CHunkMem(TInt aSize);

        TInt iSize;

        RChunk iChunk;

        TInt iBottomCommitted;
        TInt iBottomAllocated;
        
        TInt iTopCommitted;
        TInt iTopAllocated;
        

        TBool iTempOnTop;        
        TAny* iLastAllocatedTempPtr;
    
        TInt iBottomCommittedMark;
        TInt iTopCommittedMark;

        TInt iBottomAllocatedMark;
        TInt iTopAllocatedMark;
        RArray<TAny*> iTempPtrs;
    };

EXTERN_C int HunkMem_Create(int size);
EXTERN_C void HunkMem_Clear();
EXTERN_C void HunkMem_SwapBanks();
EXTERN_C void* HunkMem_Alloc(int size);
EXTERN_C void* HunkMem_AllocTemporary(int size);
EXTERN_C int HunkMem_FreeTemporary(void* ptr, int size);
EXTERN_C void HunkMem_ClearTemporary();
EXTERN_C void HunkMem_SetMark();
EXTERN_C void HunkMem_ClearToMark();

#endif

