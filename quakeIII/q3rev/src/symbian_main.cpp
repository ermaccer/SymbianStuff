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
#include <unistd.h>
#if !defined(__SYMBIAN32__)
#include <signal.h>
#endif
#include <stdlib.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <errno.h>
#ifdef __linux__ // rb010123
  #include <mntent.h>
#endif
#include <dlfcn.h>

#ifdef __linux__
  #include <fpu_control.h> // bk001213 - force dumps on divide by zero
#endif

// FIXME TTimo should we gard this? most *nix system should comply?
#if !defined(__SYMBIAN32__)
#include <termios.h>
#endif

#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../renderer/tr_public.h"

#include "symbian_local.h" // bk001204

#include <e32base.h>
#include <coemain.h>
#include "stackextender.h"

// Structure containing functions exported from refresh DLL
EXTERN refexport_t re;

unsigned  sys_frame_time;

uid_t saved_euid;
EXTERN_C qboolean stdin_active = qfalse;

CCoeEnv* theCoeEnv = NULL;
CActiveScheduler* theScheduler = NULL;



// =============================================================
// tty console variables
// =============================================================

// enable/disabled tty input mode
// NOTE TTimo this is used during startup, cannot be changed during run
// general flag to tell about tty console mode
static qboolean ttycon_on = qfalse;
// when printing general stuff to stdout stderr (Sys_Printf)
//   we need to disable the tty console stuff
// this increments so we can recursively disable
static int ttycon_hide = 0;


static field_t tty_con;

// history
// NOTE TTimo this is a bit duplicate of the graphical console history
//   but it's safer and faster to write our own here
#define TTY_HISTORY 32
static field_t ttyEditLines[TTY_HISTORY];
static int hist_current = -1, hist_count = 0;
int appOnBackground;
EXTERN_C int Symbian_ScheduleEvents(int aPriority);
// =======================================================================
// General routines
// =======================================================================
int Symbian_ScheduleEvents(int aPriority)
    {
    TInt block = appOnBackground;
    RThread thread;    
    TInt error = KErrNone;
    
    if ( block && !thread.RequestCount() )
        {
        User::WaitForAnyRequest();
        CActiveScheduler::RunIfReady( error, CActive::EPriorityIdle );
        }
    
     if (!aPriority)
        {
        TInt count = thread.RequestCount();
    while( count-- ) 
        {
        TInt res = CActiveScheduler::RunIfReady( error, aPriority );
        if (res)
            User::WaitForAnyRequest();
        }
            
        }
        else
            {
    while( thread.RequestCount() ) 
        {
        CActiveScheduler::RunIfReady( error, aPriority );
        
        User::WaitForAnyRequest();
        }
    }
    if (appOnBackground)
        {
        return 1;
        }
    return 0;
    }
// bk001207 
#define MEM_THRESHOLD 96*1024*1024

/*
==================
Sys_LowPhysicalMemory()
==================
*/
qboolean Sys_LowPhysicalMemory() {
  //MEMORYSTATUS stat;
  //GlobalMemoryStatus (&stat);
  //return (stat.dwTotalPhys <= MEM_THRESHOLD) ? qtrue : qfalse;
  return qfalse; // bk001207 - FIXME
}

/*
==================
Sys_FunctionCmp
==================
*/
int Sys_FunctionCmp(void *f1, void *f2) {
  return qtrue;
}

/*
==================
Sys_FunctionCheckSum
==================
*/
int Sys_FunctionCheckSum(void *f1) {
  return 0;
}

/*
==================
Sys_MonkeyShouldBeSpanked
==================
*/
int Sys_MonkeyShouldBeSpanked( void ) {
  return 0;
}

void Sys_BeginProfiling( void ) {
}

/*
=================
Sys_In_Restart_f

Restart the input subsystem
=================
*/
void Sys_In_Restart_f( void ) 
{
  IN_Shutdown();
  IN_Init();
}

// =============================================================
// tty console routines
// NOTE: if the user is editing a line when something gets printed to the early console then it won't look good
//   so we provide tty_Clear and tty_Show to be called before and after a stdout or stderr output
// =============================================================

// flush stdin, I suspect some terminals are sending a LOT of shit
// FIXME TTimo relevant?
void tty_FlushIn()
{
  char key;
  while (read(0, &key, 1)!=-1);
}

// do a backspace
// TTimo NOTE: it seems on some terminals just sending '\b' is not enough
//   so for now, in any case we send "\b \b" .. yeah well ..
//   (there may be a way to find out if '\b' alone would work though)
void tty_Back()
{
  char key;
  key = '\b';
  write(1, &key, 1);
  key = ' ';
  write(1, &key, 1);
  key = '\b';
  write(1, &key, 1);
}

// clear the display of the line currently edited
// bring cursor back to beginning of line
void tty_Hide()
{
  int i;
  assert(ttycon_on);
  if (ttycon_hide)
  {
    ttycon_hide++;
    return;
  }
  if (tty_con.cursor>0)
  {
    for (i=0; i<tty_con.cursor; i++)
    {
      tty_Back();
    }
  }
  ttycon_hide++;
}

// show the current line
// FIXME TTimo need to position the cursor if needed??
void tty_Show()
{
  int i;
  assert(ttycon_on);
  assert(ttycon_hide>0);
  ttycon_hide--;
  if (ttycon_hide == 0)
  {
    if (tty_con.cursor)
    {
      for (i=0; i<tty_con.cursor; i++)
      {
        write(1, tty_con.buffer+i, 1);
      }
    }
  }
}

// never exit without calling this, or your terminal will be left in a pretty bad state
void Sys_ConsoleInputShutdown()
{
  if (ttycon_on)
  {
    Com_Printf("Shutdown tty console\n");
//    tcsetattr (0, TCSADRAIN, &tty_tc);
  }
}

void Hist_Add(field_t *field)
{
  int i;
  assert(hist_count <= TTY_HISTORY);
  assert(hist_count >= 0);
  assert(hist_current >= -1);
  assert(hist_current <= hist_count);
  // make some room
  for (i=TTY_HISTORY-1; i>0; i--)
  {
    ttyEditLines[i] = ttyEditLines[i-1];
  }
  ttyEditLines[0] = *field;
  if (hist_count<TTY_HISTORY)
  {
    hist_count++;
  }
  hist_current = -1; // re-init
}

field_t *Hist_Prev()
{
  int hist_prev;
  assert(hist_count <= TTY_HISTORY);
  assert(hist_count >= 0);
  assert(hist_current >= -1);
  assert(hist_current <= hist_count);
  hist_prev = hist_current + 1;
  if (hist_prev >= hist_count)
  {
    return NULL;
  }
  hist_current++;
  return &(ttyEditLines[hist_current]);
}

field_t *Hist_Next()
{
  assert(hist_count <= TTY_HISTORY);
  assert(hist_count >= 0);
  assert(hist_current >= -1);
  assert(hist_current <= hist_count);
  if (hist_current >= 0)
  {
    hist_current--;
  }
  if (hist_current == -1)
  {
    return NULL;
  }
  return &(ttyEditLines[hist_current]);
}

// =============================================================
// general sys routines
// =============================================================

#if 0
// NOTE TTimo this is not used .. looks interesting though? protection against buffer overflow kind of stuff?
void Sys_Printf (char *fmt, ...)
{
  va_list   argptr;
  char    text[1024];
  unsigned char   *p;

  va_start (argptr,fmt);
  vsprintf (text,fmt,argptr);
  va_end (argptr);

  if (strlen(text) > sizeof(text))
    Sys_Error("memory overwrite in Sys_Printf");

  for (p = (unsigned char *)text; *p; p++)
  {
    *p &= 0x7f;
    if ((*p > 128 || *p < 32) && *p != 10 && *p != 13 && *p != 9)
      printf("[%02x]", *p);
    else
      putc(*p, stdout);
  }
}
#endif

// single exit point (regular exit or in case of signal fault)
void Sys_Exit( int ex ) {
  Sys_ConsoleInputShutdown();
  fflush(stdout);
#ifdef NDEBUG // regular behavior

  // We can't do this 
  //  as long as GL DLL's keep installing with atexit...
  //exit(ex);
  _exit(ex);
#else

  // Give me a backtrace on error exits.
  assert( ex == 0 );
  exit(ex);
#endif
}


void Sys_Quit (void) {
  CL_Shutdown ();
  fflush(stdout);
  delete theScheduler;
  theCoeEnv->DestroyEnvironment();
  fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);
  
  Sys_Exit(0);
}

void Sys_Init(void)
{
  Cmd_AddCommand ("in_restart", Sys_In_Restart_f);

  Cvar_Set( "arch", "arm" );

  Cvar_Set( "username", Sys_GetCurrentUser() );

  theCoeEnv = new CCoeEnv;
  TRAP_IGNORE(theCoeEnv->ConstructL());
    
  delete CActiveScheduler::Current();            
  theScheduler = new CActiveScheduler;    
  CActiveScheduler::Install( theScheduler );  
    
  User::SetFloatingPointMode(EFpModeRunFast);


  IN_Init();

}

void  Sys_Error( const char *error, ...)
{ 
  va_list     argptr;
  char        string[1024];

  // change stdin to non blocking
  // NOTE TTimo not sure how well that goes with tty console mode
  fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);

  // don't bother do a show on this one heh
  if (ttycon_on)
  {
    tty_Hide();
  }

  CL_Shutdown ();

  va_start (argptr,error);
  vsprintf (string,error,argptr);
  va_end (argptr);
  fprintf(stdout, "Sys_Error: %s\n", string);
  fflush(stdout);
  Sys_Exit( 1 ); // bk010104 - use single exit point.
} 

void Sys_Warn (char *warning, ...)
{ 
  va_list     argptr;
  char        string[1024];

  va_start (argptr,warning);
  vsprintf (string,warning,argptr);
  va_end (argptr);

  if (ttycon_on)
  {
    tty_Hide();
  }

  fprintf(stdout, "Warning: %s", string);
  fflush(stdout);
  if (ttycon_on)
  {
    tty_Show();
  }
} 

/*
============
Sys_FileTime

returns -1 if not present
============
*/
int Sys_FileTime (char *path)
{
  struct  stat  buf;

  if (stat (path,&buf) == -1)
    return -1;

  return buf.st_mtime;
}

void floating_point_exception_handler(int whatever)
{
//  signal(SIGFPE, floating_point_exception_handler);
}

// initialize the console input (tty mode if wanted and possible)
void Sys_ConsoleInputInit()
{
    ttycon_on = qfalse;
}

char *Sys_ConsoleInput(void)
{
 return NULL;
}

/*****************************************************************************/

/*
=================
Sys_UnloadDll

=================
*/
void Sys_UnloadDll( void *dllHandle ) {
  // bk001206 - verbose error reporting
  const char* err; // rb010123 - now const
  if ( !dllHandle )
  {
    Com_Printf("Sys_UnloadDll(NULL)\n");
    return;
  }
  dlclose( dllHandle );
  err = dlerror();
  if ( err != NULL )
    Com_Printf ( "Sys_UnloadGame failed on dlclose: \"%s\"!\n", err );
}


/*
=================
Sys_LoadDll

Used to load a development dll instead of a virtual machine
TTimo:
changed the load procedure to match VFS logic, and allow developer use
#1 look down current path
#2 look in fs_homepath
#3 look in fs_basepath
=================
*/
EXTERN_C
    {
    
    
char   *FS_BuildOSPath( const char *base, const char *game, const char *qpath );

void *Sys_LoadDll( const char *name, char *fqpath ,
                   int (**entryPoint)(int, ...),
                   int (*systemcalls)(int, ...) ) 
{
  void *libHandle;
  void  (*dllEntry)( int (*syscallptr)(int, ...) );
  char  dllname[MAX_OSPATH];
  char  fname[MAX_OSPATH];
  char  *gamedir;
  char  *fn;
  char  *tempgamedir;
  const char*  err = NULL;
	
	*fqpath = 0;

  // bk001206 - let's have some paranoia
  assert( name );

#if defined __i386__
  snprintf (fname, sizeof(fname), "%si386.so", name);
#elif defined __powerpc__   //rcg010207 - PPC support.
  snprintf (fname, sizeof(fname), "%sppc.so", name);
#elif defined __axp__
  snprintf (fname, sizeof(fname), "%saxp.so", name);
#elif defined __mips__
  snprintf (fname, sizeof(fname), "%smips.so", name);
#elif defined (__SYMBIAN32__)
  snprintf (fname, sizeof(fname), "%s-arm.dll", name);
#else
#error Unknown arch
#endif

// bk001129 - was RTLD_LAZY 
#define Q_RTLD    RTLD_NOW

  gamedir = Cvar_VariableString( "fs_game" );

  // pwdpat
  tempgamedir = FS_BuildOSPath( "", gamedir, "" );
  if (strcmp(tempgamedir, "\\baseq3\\"))
    {
    char* endseparator = strchr(tempgamedir+1,'\\');
    *endseparator = 0;
    snprintf (dllname, sizeof(dllname), "%s-%s", tempgamedir+1, fname);    
    fn = dllname;
    }
  else
    {
    fn = fname;
    }
  Com_Printf( "Sys_LoadDll(%s)... \n", fn );
  libHandle = dlopen( fn, Q_RTLD );
  if ( !libHandle )
    {
    Com_Printf ( "Sys_LoadDll(%s) failed dlopen() completely!\n", fn );
    return NULL;
    }
  Com_Printf ( "Sys_LoadDll(%s): succeeded ...\n", fn ); 

  dllEntry = (void(*)(int(*)(int,...)))dlsym( libHandle, "1" ); //[ohi] dllEntry 
  *entryPoint = (int(*)(int,...))dlsym( libHandle, "2" ); // [ohi] vmMain
  if ( !*entryPoint || !dllEntry )
  {
    err = dlerror();
#ifndef NDEBUG // bk001206 - in debug abort on failure
    Com_Error ( ERR_FATAL, "Sys_LoadDll(%s) failed dlsym(vmMain):\n\"%s\" !\n", name, err );
#else
    Com_Printf ( "Sys_LoadDll(%s) failed dlsym(vmMain):\n\"%s\" !\n", name, err );
#endif
    dlclose( libHandle );
    err = dlerror();
    if ( err != NULL )
      Com_Printf ( "Sys_LoadDll(%s) failed dlcose:\n\"%s\"\n", name, err );
    return NULL;
  }
  Com_Printf ( "Sys_LoadDll(%s) found **vmMain** at  %p  \n", name, *entryPoint ); // bk001212
  dllEntry( systemcalls );
  Com_Printf ( "Sys_LoadDll(%s) succeeded!\n", name );
  if ( libHandle ) Q_strncpyz ( fqpath , fn , MAX_QPATH ) ;		// added 7/20/02 by T.Ray
  return libHandle;
}
    }
/*
========================================================================

BACKGROUND FILE STREAMING

========================================================================
*/

#if 1

void Sys_InitStreamThread( void ) {
}

void Sys_ShutdownStreamThread( void ) {
}

void Sys_BeginStreamedFile( fileHandle_t f, int readAhead ) {
}

void Sys_EndStreamedFile( fileHandle_t f ) {
}

int Sys_StreamedRead( void *buffer, int size, int count, fileHandle_t f ) {
  return FS_Read( buffer, size * count, f );
}

void Sys_StreamSeek( fileHandle_t f, int offset, int origin ) {
  FS_Seek( f, offset, origin );
}

#else

typedef struct
{
  fileHandle_t file;
  byte  *buffer;
  qboolean  eof;
  int   bufferSize;
  int   streamPosition; // next byte to be returned by Sys_StreamRead
  int   threadPosition; // next byte to be read from file
} streamState_t;

streamState_t stream;

/*
===============
Sys_StreamThread

A thread will be sitting in this loop forever
================
*/
void Sys_StreamThread( void ) 
{
  int   buffer;
  int   count;
  int   readCount;
  int   bufferPoint;
  int   r;

  // if there is any space left in the buffer, fill it up
  if ( !stream.eof )
  {
    count = stream.bufferSize - (stream.threadPosition - stream.streamPosition);
    if ( count )
    {
      bufferPoint = stream.threadPosition % stream.bufferSize;
      buffer = stream.bufferSize - bufferPoint;
      readCount = buffer < count ? buffer : count;
      r = FS_Read ( stream.buffer + bufferPoint, readCount, stream.file );
      stream.threadPosition += r;

      if ( r != readCount )
        stream.eof = qtrue;
    }
  }
}

/*
===============
Sys_InitStreamThread

================
*/
void Sys_InitStreamThread( void ) 
{
}

/*
===============
Sys_ShutdownStreamThread

================
*/
void Sys_ShutdownStreamThread( void ) 
{
}


/*
===============
Sys_BeginStreamedFile

================
*/
void Sys_BeginStreamedFile( fileHandle_t f, int readAhead ) 
{
  if ( stream.file )
  {
    Com_Error( ERR_FATAL, "Sys_BeginStreamedFile: unclosed stream");
  }

  stream.file = f;
  stream.buffer = Z_Malloc( readAhead );
  stream.bufferSize = readAhead;
  stream.streamPosition = 0;
  stream.threadPosition = 0;
  stream.eof = qfalse;
}

/*
===============
Sys_EndStreamedFile

================
*/
void Sys_EndStreamedFile( fileHandle_t f ) 
{
  if ( f != stream.file )
  {
    Com_Error( ERR_FATAL, "Sys_EndStreamedFile: wrong file");
  }

  stream.file = 0;
  Z_Free( stream.buffer );
}


/*
===============
Sys_StreamedRead

================
*/
int Sys_StreamedRead( void *buffer, int size, int count, fileHandle_t f ) 
{
  int   available;
  int   remaining;
  int   sleepCount;
  int   copy;
  int   bufferCount;
  int   bufferPoint;
  byte  *dest;

  dest = (byte *)buffer;
  remaining = size * count;

  if ( remaining <= 0 )
  {
    Com_Error( ERR_FATAL, "Streamed read with non-positive size" );
  }

  sleepCount = 0;
  while ( remaining > 0 )
  {
    available = stream.threadPosition - stream.streamPosition;
    if ( !available )
    {
      if (stream.eof)
        break;
      Sys_StreamThread();
      continue;
    }

    bufferPoint = stream.streamPosition % stream.bufferSize;
    bufferCount = stream.bufferSize - bufferPoint;

    copy = available < bufferCount ? available : bufferCount;
    if ( copy > remaining )
    {
      copy = remaining;
    }
    memcpy( dest, stream.buffer + bufferPoint, copy );
    stream.streamPosition += copy;
    dest += copy;
    remaining -= copy;
  }

  return(count * size - remaining) / size;
}

/*
===============
Sys_StreamSeek

================
*/
void Sys_StreamSeek( fileHandle_t f, int offset, int origin ) {
  // clear to that point
  FS_Seek( f, offset, origin );
  stream.streamPosition = 0;
  stream.threadPosition = 0;
  stream.eof = qfalse;
}

#endif

/*
========================================================================

EVENT LOOP

========================================================================
*/

// bk000306: upped this from 64
#define	MAX_QUED_EVENTS		256
#define	MASK_QUED_EVENTS	( MAX_QUED_EVENTS - 1 )

sysEvent_t  eventQue[MAX_QUED_EVENTS];
// bk000306: initialize
int   eventHead = 0;
int             eventTail = 0;
byte    sys_packetReceived[MAX_MSGLEN];

/*
================
Sys_QueEvent

A time of 0 will get the current time
Ptr should either be null, or point to a block of data that can
be freed by the game later.
================
*/
void Sys_QueEvent( int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr ) {
  sysEvent_t  *ev;
  ev = &eventQue[ eventHead & MASK_QUED_EVENTS ];

  // bk000305 - was missing
  if ( eventHead - eventTail >= MAX_QUED_EVENTS )
  {
    Com_Printf("Sys_QueEvent: overflow\n");
    // we are discarding an event, but don't leak memory
    if ( ev->evPtr )
    {
      Z_Free( ev->evPtr );
    }
    eventTail++;
  }

  eventHead++;

  if ( time == 0 )
  {
    time = Sys_Milliseconds();
  }

  ev->evTime = time;
  ev->evType = type;
  ev->evValue = value;
  ev->evValue2 = value2;
  ev->evPtrLength = ptrLength;
  ev->evPtr = ptr;
}

/*
================
Sys_GetEvent

================
*/
sysEvent_t Sys_GetEvent( void ) {
  sysEvent_t  ev;
  char    *s;
  msg_t   netmsg;
  netadr_t  adr;

  // return if we have data
  if ( eventHead > eventTail )
  {
    eventTail++;
    return eventQue[ ( eventTail - 1 ) & MASK_QUED_EVENTS ];
  }

  // pump the message loop
  // in vga this calls KBD_Update, under X, it calls GetEvent
  Sys_SendKeyEvents ();

  // check for console commands
  s = Sys_ConsoleInput();
  if ( s )
  {
    char  *b;
    int   len;

    len = strlen( s ) + 1;
    b = (char*)Z_Malloc( len );
    strcpy( b, s );
    Sys_QueEvent( 0, SE_CONSOLE, 0, 0, len, b );
  }

  // check for other input devices
  IN_Frame();

  // check for network packets
  MSG_Init( &netmsg, sys_packetReceived, sizeof( sys_packetReceived ) );
  if ( Sys_GetPacket ( &adr, &netmsg ) )
  {
    netadr_t    *buf;
    int       len;

    // copy out to a seperate buffer for qeueing
    len = sizeof( netadr_t ) + netmsg.cursize;
    buf = (netadr_t*)Z_Malloc( len );
    *buf = adr;
    memcpy( buf+1, netmsg.data, netmsg.cursize );
    Sys_QueEvent( 0, SE_PACKET, 0, 0, len, buf );
  }

  // return if we have data
  if ( eventHead > eventTail )
  {
    eventTail++;
    return eventQue[ ( eventTail - 1 ) & MASK_QUED_EVENTS ];
  }

  // create an empty event to return

  memset( &ev, 0, sizeof( ev ) );
  ev.evTime = Sys_Milliseconds();

  return ev;
}

/*****************************************************************************/

qboolean Sys_CheckCD( void ) {
  return qtrue;
}

void Sys_AppActivate (void)
{
}

char *Sys_GetClipboardData(void)
{
  return NULL;
}

void  Sys_Print( const char *msg )
{
  if (ttycon_on)
  {
    tty_Hide();
  }
  fputs(msg, stdout);
  fflush(stdout);
  if (ttycon_on)
  {
    tty_Show();
  }
}


void    Sys_ConfigureFPU() { // bk001213 - divide by zero
#ifdef __linux__
#ifdef __i386
#ifndef NDEBUG

  // bk0101022 - enable FPE's in debug mode
  static int fpu_word = _FPU_DEFAULT & ~(_FPU_MASK_ZM | _FPU_MASK_IM);
  int current = 0;
  _FPU_GETCW(current);
  if ( current!=fpu_word)
  {

  }
#else // NDEBUG
  static int fpu_word = _FPU_DEFAULT;
  _FPU_SETCW( fpu_word );
#endif // NDEBUG
#endif // __i386 
#endif // __linux
}

void Sys_PrintBinVersion( const char* name ) {
  char* date = __DATE__;
  char* time = __TIME__;
  char* sep = "==============================================================";
  fprintf( stdout, "\n\n%s\n", sep );
#ifdef DEDICATED
  fprintf( stdout, "S60 Quake3 Dedicated Server [%s %s]\n", date, time );  
#else
  fprintf( stdout, "S60 Quake3 Full Executable  [%s %s]\n", date, time );  
#endif
  fprintf( stdout, " local install: %s\n", name );
  fprintf( stdout, "%s\n\n", sep );
}

void Sys_ParseArgs( int argc, char* argv[] ) {

  if ( argc==2 )
  {
    if ( (!strcmp( argv[1], "--version" ))
         || ( !strcmp( argv[1], "-v" )) )
    {
      Sys_PrintBinVersion( argv[0] );
      Sys_Exit(0);
    }
  }
}

#include "../client/client.h"
extern clientStatic_t cls;

int main ( int argc, char* argv[] )
{
  // int 	oldtime, newtime; // bk001204 - unused
  int   len, i;
  char  *cmdline;
  StackExtender::ExtendAndReplaceStack(600000);
//  void Sys_SetDefaultCDPath(const char *path);
#ifdef __WINSCW__
    freopen ("c:\\quake3\\q3_log.txt","w",stdout);
#else
    freopen ("e:\\quake3\\q3_log.txt","w",stdout);
#endif
  // go back to real user for config loads
  saved_euid = geteuid();
  seteuid(getuid());

  Sys_ParseArgs( argc, argv );  // bk010104 - added this for support
#ifdef __WINSCW__
  Sys_SetDefaultCDPath("c:\\quake3"/*argv[0]*/); //[ohi]]
#else
  Sys_SetDefaultCDPath("e:\\quake3"/*argv[0]*/); //[ohi]]
#endif
  // merge the command line, this is kinda silly
  for (len = 1, i = 1; i < argc; i++)
    len += strlen(argv[i]) + 1;
  cmdline = (char*)malloc(len);
  *cmdline = 0;
  for (i = 1; i < argc; i++)
  {
    if (i > 1)
      strcat(cmdline, " ");
    strcat(cmdline, argv[i]);
  }

  // bk000306 - clear queues
  memset( &eventQue[0], 0, MAX_QUED_EVENTS*sizeof(sysEvent_t) ); 
  memset( &sys_packetReceived[0], 0, MAX_MSGLEN*sizeof(byte) );

  Com_Init(cmdline);
  NET_Init();

  Sys_ConsoleInputInit();

  fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);
	
#ifdef DEDICATED
	// init here for dedicated, as we don't have GLimp_Init
	InitSig();
#endif

  while (1)
  {
#ifdef __linux__
    Sys_ConfigureFPU();
#endif
    
    if (Symbian_ScheduleEvents(-100))
        {
        continue;
        }
    Com_Frame ();
  }
 StackExtender::DestroyExtendedStack();

}
