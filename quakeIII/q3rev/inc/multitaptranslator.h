#ifndef __MULTITAPTRANSLATOR_H__
#define __MULTITAPTRANSLATOR_H__

#include <e32base.h>
#include <ptiengine.h>

class CMultitapKeyTranslator : public CBase
    {
    public:
        static CMultitapKeyTranslator* NewL();
        virtual ~CMultitapKeyTranslator();
        void TranslateAndSendKey(TInt aScanCode, TBool aDown, TBool aReplace);
    protected:
        CMultitapKeyTranslator();
        void ConstructL();
        void ClearCurrentWord();
    private:        
        CPtiEngine* iPtiEngine;
        TInt iPreviousCharCount;
        TChar iPreviousCharacter; 
        char iLastEnteredChar;
        TInt iPendingRight;
    };

#endif

