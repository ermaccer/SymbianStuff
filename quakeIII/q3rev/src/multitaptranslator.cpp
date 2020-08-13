#include "../ui/keycodes.h"
#include "../client/client.h"
#include "symbian_local.h"

#include "multitaptranslator.h"


CMultitapKeyTranslator* CMultitapKeyTranslator::NewL()
    {
    CMultitapKeyTranslator* self = new (ELeave)CMultitapKeyTranslator;
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
    return self;
    }
    
CMultitapKeyTranslator::CMultitapKeyTranslator()
    {
        
    }
    
CMultitapKeyTranslator::~CMultitapKeyTranslator()
    {
    delete iPtiEngine;
    }

void CMultitapKeyTranslator::ConstructL()
    {
    iPtiEngine =  CPtiEngine::NewL();
    User::LeaveIfError(iPtiEngine->ActivateLanguageL(ELangEnglish,EPtiEngineMultitapping));
    iPtiEngine->SetCase(EPtiCaseLower);
    }

void CMultitapKeyTranslator::ClearCurrentWord()
    {
    iPtiEngine->ClearCurrentWord();
    iPreviousCharCount = 0;
    iPreviousCharacter = 0; 
    iLastEnteredChar = 0;          
    iPendingRight = EFalse;
    }
    
void CMultitapKeyTranslator::TranslateAndSendKey(TInt aScanCode, TBool aDown, TBool aReplace)
    {
    TPtiKey ptiKey = EPtiKeyNone;   
    if (!aDown)
        {
        iLastEnteredChar = 0;
        return; 
        }
    
    switch (aScanCode)
        {
        case 1:
            Sys_QueEvent( 0, SE_KEY, K_BACKSPACE, qtrue, 0, NULL );
            Sys_QueEvent( 0, SE_CHAR, '\b', 0, 0, NULL );
            Sys_QueEvent( 0, SE_KEY, K_BACKSPACE, qfalse, 0, NULL );
            ClearCurrentWord();
            return;
        case EStdKeyLeftArrow: 
            Sys_QueEvent( 0, SE_KEY, K_LEFTARROW, qtrue, 0, NULL );
            Sys_QueEvent( 0, SE_KEY, K_LEFTARROW, qfalse, 0, NULL );
            return;
        case EStdKeyRightArrow: 
            Sys_QueEvent( 0, SE_KEY, K_RIGHTARROW, qtrue, 0, NULL );
            Sys_QueEvent( 0, SE_KEY, K_RIGHTARROW, qfalse, 0, NULL );
            return;
   	    case EStdKeyUpArrow:
            Sys_QueEvent( 0, SE_KEY, K_UPARROW, qtrue, 0, NULL );
            Sys_QueEvent( 0, SE_KEY, K_UPARROW, qfalse, 0, NULL );
            return;
	    case EStdKeyDownArrow:
            Sys_QueEvent( 0, SE_KEY, K_DOWNARROW, qtrue, 0, NULL );
            Sys_QueEvent( 0, SE_KEY, K_DOWNARROW, qfalse, 0, NULL );
            return;
        case EStdKeyDevice1:
            Sys_QueEvent( 0, SE_KEY, K_ESCAPE, qtrue, 0, NULL );
            Sys_QueEvent( 0, SE_KEY, K_ESCAPE, qfalse, 0, NULL );
            return;
	    case EStdKeyDevice0:
            Sys_QueEvent( 0, SE_KEY, K_ENTER, qtrue, 0, NULL );
            Sys_QueEvent( 0, SE_KEY, K_ENTER, qfalse, 0, NULL );
            return;
        case EStdKeyDevice3: 
            Sys_QueEvent( 0, SE_KEY, 'f', qtrue, 0, NULL );
            Sys_QueEvent( 0, SE_KEY, 'f', qfalse, 0, NULL );
            return;
        case EStdKeyHash:
            Sys_QueEvent( 0, SE_KEY, K_TAB, qtrue, 0, NULL );
            Sys_QueEvent( 0, SE_KEY, K_TAB, qfalse, 0, NULL );
            return;
        case 42:
    	    Sys_QueEvent( 0, SE_KEY, '~', qtrue, 0, NULL );
            Sys_QueEvent( 0, SE_KEY, '~', qfalse, 0, NULL );
            return;
        case 48:
    	    Sys_QueEvent( 0, SE_KEY, '0', qtrue, 0, NULL );
            Sys_QueEvent( 0, SE_KEY, '0', qfalse, 0, NULL );
            ptiKey = EPtiKey0;
            break;
        case 49:
    	    Sys_QueEvent( 0, SE_KEY, '1', qtrue, 0, NULL );
            Sys_QueEvent( 0, SE_KEY, '1', qfalse, 0, NULL );
            ptiKey = EPtiKey1;
            break;
        case 50:
    	    Sys_QueEvent( 0, SE_KEY, '2', qtrue, 0, NULL );
            Sys_QueEvent( 0, SE_KEY, '2', qfalse, 0, NULL );
            ptiKey = EPtiKey2;
            break;
        case 51:
    	    Sys_QueEvent( 0, SE_KEY, '3', qtrue, 0, NULL );
            Sys_QueEvent( 0, SE_KEY, '3', qfalse, 0, NULL );
            ptiKey = EPtiKey3;
            break;
        case 52:
     	    Sys_QueEvent( 0, SE_KEY, '4', qtrue, 0, NULL );
            Sys_QueEvent( 0, SE_KEY, '4', qfalse, 0, NULL );
            ptiKey = EPtiKey4;
            break;
        case 53:
     	    Sys_QueEvent( 0, SE_KEY, '5', qtrue, 0, NULL );
            Sys_QueEvent( 0, SE_KEY, '5', qfalse, 0, NULL );
            ptiKey = EPtiKey5;
            break;
        case 54:
     	    Sys_QueEvent( 0, SE_KEY, '6', qtrue, 0, NULL );
            Sys_QueEvent( 0, SE_KEY, '6', qfalse, 0, NULL );
            ptiKey = EPtiKey6;
            break;
        case 55:
    	    Sys_QueEvent( 0, SE_KEY, '7', qtrue, 0, NULL );
            Sys_QueEvent( 0, SE_KEY, '7', qfalse, 0, NULL );
            ptiKey = EPtiKey7;
            break;
        case 56:
    	    Sys_QueEvent( 0, SE_KEY, '8', qtrue, 0, NULL );
            Sys_QueEvent( 0, SE_KEY, '8', qfalse, 0, NULL );
            ptiKey = EPtiKey8;
            break;
        case 57:
    	    Sys_QueEvent( 0, SE_KEY, '9', qtrue, 0, NULL );
            Sys_QueEvent( 0, SE_KEY, '9', qfalse, 0, NULL );
            ptiKey = EPtiKey9;
            break;
        case EStdKeyApplication19: // camera key
            Sys_QueEvent( 0, SE_KEY, 'c', qtrue, 0, NULL );
            Sys_QueEvent( 0, SE_KEY, 'c', qfalse, 0, NULL );
            return;
        case EStdKeyLeftShift: // pen key
            Sys_QueEvent( 0, SE_KEY, 's', qtrue, 0, NULL );
            Sys_QueEvent( 0, SE_KEY, 's', qfalse, 0, NULL );
            return;
        case EStdKeyYes: // call answer key
            Sys_QueEvent( 0, SE_KEY, 'y', qtrue, 0, NULL );
            Sys_QueEvent( 0, SE_KEY, 'y', qfalse, 0, NULL );
            return;
        
	    case 133:
            ptiKey = EPtiKeyStar;
            break;

        default:
            ClearCurrentWord();
            return;
        }
    TInt wordLen = 0;
    iPtiEngine->AppendKeyPress(ptiKey);
    wordLen = iPtiEngine->CurrentWord().Length();
    if (!wordLen)
        {
        ClearCurrentWord();
        return;
        }
    TChar lastChar = iPtiEngine->CurrentWord().operator[](wordLen-1);

    if (wordLen > iPreviousCharCount)
        {
        
        TBuf8<256> temp;
        temp.Copy(iPtiEngine->CurrentWord());
        iLastEnteredChar = temp[wordLen-1];
        iPreviousCharacter = iPtiEngine->CurrentWord().operator[](wordLen-1);
        if (wordLen < iPreviousCharCount || iPreviousCharacter > 126)
            {
            ClearCurrentWord();
            return;
            }
        iPreviousCharCount++;
        Sys_QueEvent( 0, SE_CHAR, iLastEnteredChar, 0, 0, NULL );
        if (aReplace)
            {
            iPendingRight = ETrue;
            }
        }
    else if (wordLen > 0 && lastChar != iPreviousCharacter)
        {
        TBuf8<256> temp;
        temp.Copy(iPtiEngine->CurrentWord());
        iLastEnteredChar = temp[wordLen-1];
        iPreviousCharacter = iPtiEngine->CurrentWord().operator[](wordLen-1);
        if (wordLen < iPreviousCharCount || iPreviousCharacter > 126 )
            {
            ClearCurrentWord();
            return;
            }
  
        if (aReplace && iPendingRight)
            {
            Sys_QueEvent( 0, SE_KEY, K_RIGHTARROW, qtrue, 0, NULL );
            Sys_QueEvent( 0, SE_KEY, K_RIGHTARROW, qfalse, 0, NULL ); 
            }
        Sys_QueEvent( 0, SE_KEY, K_LEFTARROW, qtrue, 0, NULL );
        Sys_QueEvent( 0, SE_KEY, K_LEFTARROW, qfalse, 0, NULL ); 
        // console uses insert...
        if (!aReplace)
            {        
            Sys_QueEvent( 0, SE_KEY, K_DEL, qtrue, 0, NULL );
            Sys_QueEvent( 0, SE_KEY, K_DEL, qfalse, 0, NULL ); 
            }
            
        Sys_QueEvent( 0, SE_CHAR, iLastEnteredChar, 0, 0, NULL );
        if (aReplace)
            {
            Sys_QueEvent( 0, SE_KEY, K_RIGHTARROW, qtrue, 0, NULL );
            Sys_QueEvent( 0, SE_KEY, K_RIGHTARROW, qfalse, 0, NULL ); 
            }
        iPendingRight = EFalse;
        }    
    }

