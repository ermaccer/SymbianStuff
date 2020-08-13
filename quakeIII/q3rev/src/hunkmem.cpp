#include "hunkmem.h"
#include "q_shared.h"

CHunkMem::CHunkMem(TInt aSize) : iSize(aSize) 
    {
    }

CHunkMem::~CHunkMem()
    {
    iChunk.Close();
    }
    
CHunkMem* CHunkMem::NewL(TInt aSize)
    {
    CHunkMem* self = new (ELeave) CHunkMem(aSize);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
    return self;
    }
    
void CHunkMem::ConstructL()
    {
    User::LeaveIfError(iChunk.CreateDisconnectedLocal(0,0,iSize));
    }
    
TAny* CHunkMem::Alloc(TInt aSize)
    {
    TInt offset = 0;
    TInt size = 0;
    TAny* ptr = NULL;
    TInt err = KErrNone;
    if (!iTempOnTop)
        {
        // temp is on bottom -> allocate permanent memory from top
        if ((iTopAllocated+aSize) > iTopCommitted)
            {
            size = (iTopAllocated+aSize) - iTopCommitted;
            ALIGN_TO_PAGESIZE(size);
            iTopCommitted+=size;
            offset = iSize-iTopCommitted;
            err = iChunk.Commit(offset,size);
            if (err)
                {
                Com_Error( ERR_DROP, "CHunkMem::Alloc failed on %i", aSize );  
                return NULL;
                }
            }
        iTopAllocated+=aSize;
        ptr = (iChunk.Base() + iSize) - iTopAllocated;
        }
    else
        {
        // temp is on top -> allocate permanent memory from bottom
        if ((iBottomAllocated+aSize) > iBottomCommitted)
            {
            // need to commit more bytes to the chunk
            offset = iBottomCommitted;
            size = (iBottomAllocated+aSize) - iBottomCommitted;
            ALIGN_TO_PAGESIZE(size);
            err = iChunk.Commit(offset, size);
            if (err)
                {
                Com_Error( ERR_DROP, "CHunkMem::Alloc failed on %i", aSize );  
                return NULL;
                }

            iBottomCommitted+=size;
            }
        ptr = iChunk.Base() + iBottomAllocated;
        iBottomAllocated+=aSize;
        }
    return ptr;
    }
    
TAny* CHunkMem::AllocTemporary(TInt aSize)
    {
    TInt offset = 0;
    TInt size = 0;
    TAny* ptr = NULL;
    TInt err = KErrNone;
    if (!iTempOnTop)
        {
        // temp is on bottom -> allocate temp memory from bottom
        if ((iBottomAllocated+aSize) > iBottomCommitted)
            {
            // need to commit more bytes to the chunk
            offset = iBottomCommitted;
            size = (iBottomAllocated+aSize) - iBottomCommitted;
            ALIGN_TO_PAGESIZE(size);
            err = iChunk.Commit(offset, size);
            if (err)
                {
                Com_Error( ERR_DROP, "CHunkMem::AllocTemporary failed on %i", aSize );    
                return NULL;
                }
            iBottomCommitted+=size;
            }
        ptr = iChunk.Base() + iBottomAllocated;
        iBottomAllocated+=aSize;
        }
    else
        {
        // temp is on top -> allocate temp memory from top
        if ((iTopAllocated+aSize) > iTopCommitted)
            {
            size = (iTopAllocated+aSize) - iTopCommitted;
            ALIGN_TO_PAGESIZE(size);
            iTopCommitted+=size;
            offset = iSize-iTopCommitted;
            err = iChunk.Commit(offset,size);
            if (err)
                {
                Com_Error( ERR_DROP, "CHunkMem::AllocTemporary failed on %i", aSize );  
                return NULL;
                }
            }
        iTopAllocated+=aSize;
        ptr = (iChunk.Base() + iSize) - iTopAllocated;
        }
    iTempPtrs.Append((TAny*)(ptr));
    return ptr;
    }
    
TInt CHunkMem::FreeTemporary(TAny* aPtr, TInt aSize)
    {
    TInt ptrCount = iTempPtrs.Count();
    if (ptrCount)
        {
        TAny* ptr = (TAny*)iTempPtrs[ptrCount-1];
        if (ptr == aPtr)
            {
            iTempPtrs.Remove(ptrCount-1);
            }
        else
            {
            return 0;
            }
        }
    else
        {
        return 0;
        }

    TInt freespace = 0;        
    if (!iTempOnTop)
        {
        // temp is on bottom
        iBottomAllocated-=aSize;
        freespace = iBottomCommitted-iBottomAllocated;
        ROUND_TO_PAGESIZE(freespace);
        if (freespace)
            {
            iBottomCommitted-=freespace;
            iChunk.Decommit(iBottomCommitted, freespace);            
            }
        }
    else
        {
        // temp is on top
        iTopAllocated -=aSize;
        freespace = iTopCommitted-iTopAllocated;
        ROUND_TO_PAGESIZE(freespace);
        if (freespace)
            {
            iChunk.Decommit(iSize-iTopCommitted,freespace);
            iTopCommitted -= freespace;
            }
        }
    return 1;
    }

void CHunkMem::ClearTemporary()
    {
    TInt offset = 0;
    TInt size = 0;
    TInt err = KErrNone;
    if (!iTempOnTop)
        {
        // temp is on bottom
        size = iBottomCommitted;
        iBottomAllocated = 0;
        iBottomCommitted = 0;
        }
    else
        {
        // temp is on top
        offset = iSize-iTopCommitted;
        size = iTopCommitted;
        iTopAllocated = 0;
        iTopCommitted = 0;
        }
    if (size)
        {
        err = iChunk.Decommit(offset, size);        
        }
//    iLastAllocatedTempPtr = NULL;
    iTempPtrs.Reset();
    }
    
void CHunkMem::SwapBanks()
    {
    // switch the temporary/permanent banks
    iTempOnTop^=1;
    }

void CHunkMem::Clear()
    {
    if (iBottomCommitted)
        {
        iChunk.Decommit(0,iBottomCommitted);
        iBottomCommitted = 0;
        iBottomAllocated = 0;
        }
    if (iTopCommitted)
        {
        iChunk.Decommit(iSize-iTopCommitted, iTopCommitted);
        iTopAllocated = 0;
        iTopCommitted = 0;
        }
    iBottomCommittedMark = 0;
    iBottomAllocatedMark = 0;
    iTopCommittedMark = 0;
    iTopAllocatedMark = 0;
    iTempPtrs.Reset();
    }

void CHunkMem::SetMark()
    {
    iBottomCommittedMark = iBottomCommitted;
    iBottomAllocatedMark = iBottomAllocated;
    iTopCommittedMark = iTopCommitted;
    iTopAllocatedMark = iTopAllocated;
    }
    
void CHunkMem::ClearToMark()
    {
    if (iBottomCommittedMark)
        {
        if (iBottomCommitted-iBottomCommittedMark)
            {
            iChunk.Decommit(iBottomCommittedMark, iBottomCommitted-iBottomCommittedMark);
            }
        iBottomCommitted=iBottomCommittedMark;
        iBottomAllocated=iBottomAllocatedMark;
        }
    if (iTopCommittedMark)
        {
        if (iTopCommitted-iTopCommittedMark)
            {
            iChunk.Decommit(iSize-iTopCommitted, iTopCommitted-iTopCommittedMark);
            }
        iTopCommitted=iTopCommittedMark;
        iTopAllocated=iTopAllocatedMark;
        }
    iBottomCommittedMark = 0;
    iBottomAllocatedMark = 0;
    iTopCommittedMark = 0;
    iTopAllocatedMark = 0;
    }

CHunkMem* theChunkMem = NULL;


int HunkMem_Create(int size)
    {
    TRAPD(error, theChunkMem = CHunkMem::NewL(size));
    return error;
    }
    
void HunkMem_Clear()
    {
    theChunkMem->Clear();
    }
    
void HunkMem_SwapBanks()
    {
    theChunkMem->SwapBanks();
    }
    
void* HunkMem_Alloc(int size)
    {
    return theChunkMem->Alloc(size);
    }
    
void* HunkMem_AllocTemporary(int size)
    {
    return theChunkMem->AllocTemporary(size);
    }
    
int HunkMem_FreeTemporary(void* ptr, int size)
    {
    return theChunkMem->FreeTemporary(ptr, size);
    }
    
void HunkMem_ClearTemporary()
    {
    return theChunkMem->ClearTemporary();
    }

void HunkMem_SetMark()
    {
    theChunkMem->SetMark();
    }
    
void HunkMem_ClearToMark()
    {
    theChunkMem->ClearToMark();
    }



