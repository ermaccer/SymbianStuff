/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
#include "../renderer/tr_local.h"
#include "../ui/keycodes.h"
#include "../client/client.h"
#include "symbian_local.h"

#include <w32std.h>
#include <APGWGNAM.H>
#include <coemain.h>

#include <gl/egl.h>
#include <nanogl.h>

#include "quake3_uid.h"
#include "multitaptranslator.h"
#include "eventmonitor.h"
#include "remotectrlmonitor.h"

RWindowGroup				wsWindowGroup;
RWindow						wsWindow;
CApaWindowGroupName*        wsWindGroupName;
CApaWindowGroupName*        rootWindGroupName;

CMultitapKeyTranslator* theKeyTranslator = NULL;
CQuakeEventMonitor* theMonitor = NULL;
CRemoteCtrlEventMonitor* theRemCon = NULL;

EGLDisplay sglDisplay;
EGLConfig  sglConfig;
EGLContext sglContext;
EGLSurface sglSurface;
TBool windowCreated = EFalse;
extern int appOnBackground;




extern "C" qboolean ( * qwglSwapIntervalEXT)( int interval ) = NULL;
extern "C" void ( * qglMultiTexCoord2fARB )( GLenum texture, float s, float t ) = NULL;
extern "C" void ( * qglActiveTextureARB )( GLenum texture ) = NULL;
extern "C" void ( * qglClientActiveTextureARB )( GLenum texture ) = NULL;


extern "C" void ( * qglLockArraysEXT)( int, int) = NULL;
extern "C" void ( * qglUnlockArraysEXT) ( void ) = NULL;


void CreateWindow()
    {
    CWsScreenDevice* screenDev = CCoeEnv::Static()->ScreenDevice();
    RWsSession wsSession = CCoeEnv::Static()->WsSession();
	TSize displaySize = screenDev->SizeInPixels();

    
    wsWindowGroup=RWindowGroup(wsSession);
    wsWindowGroup.Construct((TUint32)&wsWindowGroup);
	wsWindowGroup.SetOrdinalPosition(0);
    wsWindowGroup.EnableScreenChangeEvents();
    wsWindowGroup.EnableReceiptOfFocus(ETrue);
    
    wsWindow=RWindow(wsSession);
	wsWindow.Construct(wsWindowGroup, (TUint32)&wsWindow);
	wsWindow.Activate();
	wsWindow.SetSize(displaySize);
    wsWindow.SetVisible(ETrue);

    TRAP_IGNORE(wsWindGroupName = CApaWindowGroupName::NewL(wsSession, wsWindowGroup.Identifier()));
    TUid wgUid;
    wgUid.iUid = QUAKE3_APPUID;
    wsWindGroupName->SetAppUid(wgUid);
    wsWindGroupName->SetCaptionL(_L("Quake3Arena"));
    
    wsWindGroupName->SetHidden(EFalse);
    wsWindGroupName->SetAppReady(ETrue);
    wsWindGroupName->SetSystem(EFalse);
    wsWindGroupName->SetWindowGroupName(wsWindowGroup);

    RWindowGroup rootWin = CCoeEnv::Static()->RootWin();
    TRAP_IGNORE(rootWindGroupName = CApaWindowGroupName::NewL(wsSession, rootWin.Identifier()));
    rootWindGroupName->SetHidden(ETrue);
    rootWindGroupName->SetWindowGroupName(rootWin);
    TRAP_IGNORE(theMonitor = CQuakeEventMonitor::NewL());
    TRAP_IGNORE(theKeyTranslator = CMultitapKeyTranslator::NewL());
    TRAP_IGNORE(theRemCon = CRemoteCtrlEventMonitor::NewL());
    windowCreated = ETrue;
    }

void		GLimp_EndFrame( void ) 
    {
	eglSwapBuffers(sglDisplay,sglSurface);
    User::ResetInactivityTime();
    }

/*
** GLW_InitExtensions
*/
void InitGLExtensions( void )
{
	if ( !r_allowExtensions->integer )
	{
		ri.Printf( PRINT_ALL, "*** IGNORING OPENGL EXTENSIONS ***\n" );
		return;
	}

	ri.Printf( PRINT_ALL, "Initializing OpenGL extensions\n" );

	// GL_S3_s3tc
	glConfig.textureCompression = TC_NONE;
	if ( strstr( glConfig.extensions_string, "GL_S3_s3tc" ) )
	{
		if ( r_ext_compressed_textures->integer )
		{
			glConfig.textureCompression = TC_S3TC;
			ri.Printf( PRINT_ALL, "...using GL_S3_s3tc\n" );
		}
		else
		{
			glConfig.textureCompression = TC_NONE;
			ri.Printf( PRINT_ALL, "...ignoring GL_S3_s3tc\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_S3_s3tc not found\n" );
	}

	// GL_EXT_texture_env_add
	glConfig.textureEnvAddAvailable = qfalse;
	if ( strstr( glConfig.extensions_string, "EXT_texture_env_add" ) )
	{
		if ( r_ext_texture_env_add->integer )
		{
			glConfig.textureEnvAddAvailable = qtrue;
			ri.Printf( PRINT_ALL, "...using GL_EXT_texture_env_add\n" );
		}
		else
		{
			glConfig.textureEnvAddAvailable = qfalse;
			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_texture_env_add\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_texture_env_add not found\n" );
	}

	// GL_ARB_multitexture
	qglMultiTexCoord2fARB = NULL;
	qglActiveTextureARB = NULL;
	qglClientActiveTextureARB = NULL;
	if ( strstr( glConfig.extensions_string, "GL_ARB_multitexture" )  )
	{
		if ( r_ext_multitexture->integer )
		{
			qglMultiTexCoord2fARB = (void (*)(GLenum, float, float))eglGetProcAddress( "glMultiTexCoord2fARB" );
			qglActiveTextureARB =  (void (*)(GLenum))eglGetProcAddress( "glActiveTextureARB" );
			qglClientActiveTextureARB = (void (*)(GLenum))eglGetProcAddress( "glClientActiveTextureARB" );

			if ( qglActiveTextureARB )
			{
				qglGetIntegerv( GL_MAX_ACTIVE_TEXTURES_ARB, &glConfig.maxActiveTextures );

				if ( glConfig.maxActiveTextures > 1 )
				{
					ri.Printf( PRINT_ALL, "...using GL_ARB_multitexture\n" );
				}
				else
				{
					qglMultiTexCoord2fARB = NULL;
					qglActiveTextureARB = NULL;
					qglClientActiveTextureARB = NULL;
					ri.Printf( PRINT_ALL, "...not using GL_ARB_multitexture, < 2 texture units\n" );
				}
			}
		}
		else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_ARB_multitexture\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_ARB_multitexture not found\n" );
	}

}


EGLint attrib_list_fsaa[] =
        {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BUFFER_SIZE,  0,
        EGL_DEPTH_SIZE,   16,
  	    EGL_SAMPLE_BUFFERS, 1,
    	EGL_SAMPLES,        4,
        EGL_NONE
        };

EGLint attrib_list[] =
        {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BUFFER_SIZE,  0,
        EGL_DEPTH_SIZE,   16,
        EGL_NONE
        };

void 		GLimp_Init( void )
    {
    ri.Printf( PRINT_ALL, "Initializing OpenGL subsystem\n" );
    CreateWindow();
    nanoGL_Init();

    EGLint *attribList = NULL;
	if (r_fsaa->integer)    
		{
        ri.Printf( PRINT_ALL, "....Using Full Scene Antialiasing\n" );
		attribList = attrib_list_fsaa;
		}
	else
		{
		attribList = attrib_list;
		}

	switch( wsWindow.DisplayMode() )
		{
		case( EColor4K ) : { attribList[3] = 12; break; }
		case( EColor64K ): { attribList[3] = 16; break; }
		case( EColor16M ): { attribList[3] = 24; break; }
		default: attribList[3] = 32; // for EColor16MU
		}	

	EGLint numConfigs;
	EGLint majorVersion;
	EGLint minorVersion;
	
	sglDisplay = eglGetDisplay( EGL_DEFAULT_DISPLAY );
	if( sglDisplay == EGL_NO_DISPLAY )
		{
	    ri.Printf( PRINT_ALL, "GL No Display\n" );
		User::Panic( _L("GL No Display"),0 );
		}
	if( !eglInitialize( sglDisplay, &majorVersion, &minorVersion ) ) 
		{
	    ri.Printf( PRINT_ALL, "GL Init\n" );
		User::Panic( _L("GL Init"), 0 );
		}
	if( !eglChooseConfig( sglDisplay, attribList, &sglConfig, 1, &numConfigs ) )
		{
        ri.Printf( PRINT_ALL, "GL Config\n" );
		User::Panic(_L("GL Config"), 0 );
		}
	sglContext = eglCreateContext( sglDisplay, sglConfig, NULL, NULL );
	if( sglContext==0 )
		{
       ri.Printf( PRINT_ALL, "GL Context\n" );
		User::Panic( _L("GL Context"), 0 );
		}
	sglSurface = eglCreateWindowSurface( sglDisplay, sglConfig, &wsWindow, NULL );
	if( sglSurface==0 )
		{
        ri.Printf( PRINT_ALL, "GL surface\n" );
		User::Panic( _L("GL Surface"), 0 );
		}
	
	eglMakeCurrent( sglDisplay, sglSurface, sglSurface, sglContext );    
    qglHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    
    // get our config strings
	Q_strncpyz( glConfig.vendor_string, (const char*)qglGetString (GL_VENDOR), sizeof( glConfig.vendor_string ) );
	Q_strncpyz( glConfig.renderer_string, (const char*)qglGetString (GL_RENDERER), sizeof( glConfig.renderer_string ) );
	Q_strncpyz( glConfig.version_string, (const char*)qglGetString (GL_VERSION), sizeof( glConfig.version_string ) );
	Q_strncpyz( glConfig.extensions_string, (const char*)qglGetString (GL_EXTENSIONS), sizeof( glConfig.extensions_string ) );
	
    ri.Printf( PRINT_ALL, "...setting mode %d:", 0 );
	
//	R_GetModeInfo( &glConfig.vidWidth, &glConfig.vidHeight, &glConfig.windowAspect, 0 );
    CWsScreenDevice* screenDev = CCoeEnv::Static()->ScreenDevice();
    TPixelsTwipsAndRotation rot;
    screenDev->GetScreenModeSizeAndRotation(screenDev->CurrentScreenMode(),rot);
    TSize displaySize = rot.iPixelSize;
    glConfig.vidWidth = displaySize.iWidth;
    glConfig.vidHeight = displaySize.iHeight;
	r_mode->integer = 0;
    glConfig.colorBits = 32;
    glConfig.depthBits = 16;
    glConfig.stencilBits = 0;
    
    InitGLExtensions();
    }

void		GLimp_Shutdown( void ) 
    {
        
    delete theKeyTranslator; 
    theKeyTranslator = NULL;
    delete theMonitor; 
    theMonitor = NULL;
    delete theRemCon;
    theRemCon = NULL;
    delete rootWindGroupName;
    rootWindGroupName = NULL;
    delete wsWindGroupName;
    wsWindGroupName = NULL;
    
    wsWindow.Close();
    wsWindowGroup.Close();

    windowCreated = EFalse;

    eglMakeCurrent( sglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
    eglDestroySurface( sglDisplay, sglSurface ); 
    eglDestroyContext( sglDisplay, sglContext );
    eglTerminate( sglDisplay );   
    nanoGL_Destroy();
    
    memset( &glConfig, 0, sizeof( glConfig ) );
	memset( &glState, 0, sizeof( glState ) );
    }

void		GLimp_EnableLogging( qboolean enable ) 
    {
    }

void GLimp_LogComment( char *comment ) 
    {
    }

qboolean QGL_Init( const char *dllname ) 
    {
	return qtrue;
    }

void		QGL_Shutdown( void ) 
    {
    }


void *GLimp_RendererSleep( void ) { return NULL;}
extern "C" qboolean GLimp_SpawnRenderThread( void (*function)( void ) ) { return qfalse;}
void GLimp_FrontEndSleep( void ) {}
void GLimp_WakeRenderer( void *data ) {}
void GLimp_SetGamma( unsigned char red[256], unsigned char green[256], unsigned char blue[256] ) {}
