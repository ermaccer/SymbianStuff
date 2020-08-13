#include <w32std.h>
#include <aknutils.h>
#include <aknutils.h>

#include "../renderer/tr_local.h"
#include "../ui/keycodes.h"
#include "../client/client.h"
#include "symbian_local.h"

#include "multitaptranslator.h"

extern TBool windowCreated;
extern int appOnBackground;
extern RWindow wsWindow;

extern CMultitapKeyTranslator* theKeyTranslator;

// missing ö,ä,å,chr

static qboolean shift_down = qfalse;

int scan_to_quake[256] =
    {
    /*  00  */ 000,K_BACKSPACE,K_TAB,K_ENTER,000,K_SPACE,000,000,000,000,000,000,000,000,K_LEFTARROW,K_RIGHTARROW,
    /*  16  */ K_UPARROW,K_DOWNARROW,K_SHIFT,K_SHIFT,000,000,K_CTRL,000,000,000,000,000,000,000,000,000,
    /*  32  */ 000,000,000,000,000,000,000,000,000,000,000,'+',000,000,000,000,
    /*  48  */ '0','1','2','3','4','5','6','7','8','9',000,000,000,000,000,000,
    /*  64  */ 000,'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o',
    /*  80  */ 'p','q','r','s','t','u','v','w','x','y','z',000,000,000,000,000,
    /*  96  */ 000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,
    /*  112 */ 000,000,000,000,000,000,000,000,000,',','.',000,'<',000,000,'\'',
    /*  128 */ '\\',000,'-',000,000,000,000,000,000,000,000,000,000,000,000,000,
    /*  144 */ 000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,
    /*  160 */ 000,000,000,000,K_ENTER,K_ESCAPE,000,K_KP_ENTER,000,000,000,000,000,000,000,000,
    /*  176 */ 000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,
    /*  192 */ 000,000,000,000,'~',000,000,000,000,000,000,000,000,000,000,000,
    /*  208 */ 000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,
    /*  224 */ 000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,
    /*  240 */ 000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000
    };

int scan_to_quake_shifted[256] =
    {
    /*  00  */ 000,K_BACKSPACE,K_TAB,K_ENTER,000,K_SPACE,000,000,000,000,000,000,000,000,K_LEFTARROW,K_RIGHTARROW,
    /*  16  */ K_UPARROW,K_DOWNARROW,K_SHIFT,K_SHIFT,000,000,K_CTRL,000,000,000,000,000,000,000,000,000,
    /*  32  */ 000,000,000,000,000,000,000,000,000,000,000,'?',000,000,000,000,
    /*  48  */ '=','!','"','#','$','%','&','/','(',')',000,000,000,000,000,000,
    /*  64  */ 000,'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O',
    /*  80  */ 'P','Q','R','S','T','U','V','W','X','Y','Z',000,000,000,000,000,
    /*  96  */ 000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,
    /*  112 */ 000,000,000,000,000,000,000,000,000,';',':',000,'>',000,000,'*',
    /*  128 */ '|',000,'_',000,000,000,000,000,000,000,000,000,000,000,000,000,
    /*  144 */ 000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,
    /*  160 */ 000,000,000,000,K_ENTER,K_ESCAPE,000,K_KP_ENTER,000,000,000,000,000,000,000,000,
    /*  176 */ 000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,
    /*  192 */ 000,000,000,000,'~',000,000,000,000,000,000,000,000,000,000,000,
    /*  208 */ 000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,
    /*  224 */ 000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,
    /*  240 */ 000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000
    };



int MapKey (int code)
    {
    switch (code)
        {
        case EStdKeyLeftArrow: return K_LEFTARROW;
        case EStdKeyRightArrow: return K_RIGHTARROW;
	    case EStdKeyUpArrow: return K_UPARROW;
	    case EStdKeyDownArrow: return K_DOWNARROW;
        case EStdKeyDevice1: return K_ESCAPE;
	    case EStdKeyDevice0: return K_ENTER;
	    case EStdKeyDevice3: return 'f';
	    case 1: return K_BACKSPACE;
        case EStdKeyHash: return K_TAB;
        case 42: return '~';
        case 48: return '0';
        case 49: return '1';
        case 50: return '2';
        case 51: return '3';
        case 52: return '4';
        case 53: return '5';
        case 54: return '6';
        case 55: return '7';
        case 56: return '8';
        case 57: return '9';
        case EStdKeyApplication19: return 'c'; // camera key
        case EStdKeyLeftShift: return 's'; // pen key
        case EStdKeyYes: return 'y'; // call answer key
	    default: return 0;
        }
    }

TBool IsQwerty()
    {
    if ((cls.glconfig.vidWidth == 800 && cls.glconfig.vidHeight == 352)  // e90
        ||(cls.glconfig.vidWidth == 640 && cls.glconfig.vidHeight == 480)) // e6
        {
        return ETrue;
        }
    return EFalse;
    }

void HandleWsEvent( const TWsEvent& aWsEvent )
    {
    TInt eventType = aWsEvent.Type();
    TBool down = EFalse;
    TKeyEvent* event;

    switch( eventType )
        {
        case EEventKeyDown:
            {
            //Com_Printf( "scancode, code %d\n",aWsEvent.Key()->iScanCode);
            down = ETrue;
            } // fall through
        case EEventKeyUp:
            {
            TInt scan = 0;
            TBool isqwerty = IsQwerty();
            event = aWsEvent.Key();
            if (isqwerty)
                {
                if (shift_down)
                    {
                    scan = scan_to_quake_shifted[event->iScanCode&0xff];
                    }
                else
                    {
                    scan = scan_to_quake[event->iScanCode&0xff];
                    }
                if (scan == K_SHIFT)
                   {
                   shift_down = (qboolean)down;
                   }
                }
            else
                {
                scan = MapKey(event->iScanCode);
                }
            TBool replace = EFalse;
            // console and ui should be able to receive text typed by the phone keys..
            // also, this statement should check whether we are (re)configuring the keys..
            if (!isqwerty && ((cls.keyCatchers & KEYCATCH_CONSOLE) || (cls.keyCatchers & KEYCATCH_UI)))
                {
                if (cls.keyCatchers & KEYCATCH_UI && !(cls.keyCatchers & KEYCATCH_CONSOLE))
                    {
                    replace = ETrue;
                    }
                // ui uses always replace mode???
                theKeyTranslator->TranslateAndSendKey(event->iScanCode,down, replace);
                }
            // no typing needed in the game or when (re)configuring the keys
            else if (down)
                {
                Sys_QueEvent(0, SE_KEY, scan ,qtrue,0,NULL);                
                if (isqwerty && scan <= 127)
                    {
                    if (scan == K_BACKSPACE)
                        {
                        scan = '\b';
                        }
                    Sys_QueEvent(0, SE_CHAR, scan, 0, 0, NULL );
                    }
                }
            else
                {
                Sys_QueEvent(0, SE_KEY, scan ,qfalse,0,NULL);
                }            
            break;
            }
            
        case EEventScreenDeviceChanged:
            {
            if (windowCreated)
                {
                CWsScreenDevice* screenDev = CCoeEnv::Static()->ScreenDevice();
                TPixelsTwipsAndRotation rot;
                screenDev->GetScreenModeSizeAndRotation(screenDev->CurrentScreenMode(),rot);
                TSize displaySize = rot.iPixelSize;
            	wsWindow.SetSize(displaySize);
            	wsWindow.SetExtent(TPoint(0,0),displaySize);
                glConfig.vidWidth = displaySize.iWidth;
                glConfig.vidHeight = displaySize.iHeight;    
                cls.glconfig = glConfig;
                if (cls.uiStarted)
                    {
                    VM_Call(uivm, UI_UPDATE_GLCONFIG);
                    }
                if (cls.state == CA_ACTIVE)
                    {
                    VM_Call(cgvm, CG_UPDATE_GLCONFIG);
                    }
                }
            break;                    
            }
        case EEventFocusLost:
            {
            appOnBackground = 1;
            break;
            }
        case EEventFocusGained:
            {
            appOnBackground = 0;
            SetKeyblockMode( ENoKeyBlock );
            break; 
            }            
        default:
            break;
        }        
    }
