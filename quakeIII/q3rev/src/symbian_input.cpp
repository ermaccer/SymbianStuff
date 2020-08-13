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
#include "../client/client.h"
#include "symbian_local.h"
#include <e32base.h>
#include "hidinputmonitor.h"

CHIDEventMonitor* theHIDMonitor = NULL;
TPoint currentPosition(0,0);
qboolean mouseactive;

void IN_Init( void ) 
    {
    Com_Printf( "Input initialization...\n" );
    Com_Printf( "  ... Connecting to HID server & creating event listener\n" );
    TRAPD(err, theHIDMonitor = CHIDEventMonitor::NewL());
    if (err)
        {
        Com_Printf( "  ... Failed with error %d\n", err );    
        }
    else
        {
        Com_Printf( "  ... HID server connected, listening for events\n", err );    
        }
    currentPosition = TPoint( cls.glconfig.vidWidth /2, cls.glconfig.vidHeight /2);
    mouseactive = qtrue;
    Com_Printf( "Input initialization complete\n");    
    }

void IN_MouseMove()
    {
    int	mx, my;

    // don't bother sending events it mouse is not active or has not moved...
    if (!mouseactive || 
        currentPosition == TPoint( cls.glconfig.vidWidth /2, cls.glconfig.vidHeight /2))
		return;

	
	mx = currentPosition.iX - cls.glconfig.vidWidth /2;
	my = currentPosition.iY - cls.glconfig.vidHeight /2;

    // this can possibly crash the game if called too early in init...
   	Sys_QueEvent( 0, SE_MOUSE, mx, my, 0, NULL ); 

    currentPosition = TPoint( cls.glconfig.vidWidth /2, cls.glconfig.vidHeight /2);
    }

void IN_Frame (void) 
    {
    IN_MouseMove();
    }

void IN_Shutdown( void ) 
    {
    mouseactive = qfalse;
    delete theHIDMonitor;
    }

void Sys_SendKeyEvents (void) 
    {
    }

