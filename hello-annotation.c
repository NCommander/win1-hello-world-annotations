/*  Hello.c
    Hello Application
    Windows Toolkit Version 1.03
    Copyright (c) Microsoft 1985,1986        */

/**
 * Annotations by Michael Casadevall (NCommander) - May 3rd 2020
 * 
 * Annotations marked by NC: in the source
 */

/**
 * NC: The original Hello World program in all it's glory, as printed
 * in multiple versions of Programming Windows, and versions of this
 * remained in the SDK for years.
 * 
 * Given it's vintage, keep in mind, this is K&R C, which means things
 * line inline variable declarations for C functions simply don't exist.
 * 
 * In addition, this is real mode 16-bit programming, which means that
 * we're dealing with a segmented address space. I'll write more about
 * segments later, but just note the NEAR and FAR do infact mean something
 * in this version of Windows
 */


/**
 * NC: Normally, I won't weigh in on header files, but the Windows header is one file
 * in total. And it lacks include guards! (these weren't a new concept even then!)
 */

#include "windows.h"
#include "hello.h"


/**
 * NC: I've re-arranged the source code blocks in this file to make easier reading
 * as I need to introduce some concepts. As such, this won't compile "as is" on
 * MSC 4, but the code is unchanged from HELLO.C, so if you're mad enough to build
 * this yourself, go use that!
 */

/**
 * NC: Globals, and forward decls. Even back then, MSFT used Hungarian
 * notation everywhere
 */

char szAppName[10];
char szAbout[10];
char szMessage[15];
int MessageLength;

static HANDLE hInst;
FARPROC lpprocAbout;

long FAR PASCAL HelloWndProc(HWND, unsigned, WORD, LONG);

/**
 * NC: The WinMan prototype.
 * 
 * This has remained unchanged in modern versions of Windows, and is the entry point
 * for all C based Windows programming.
 * 
 * PASCAL is what would become __stdcall in later C compilers. My assumption is since
 * PASCAL was a first class citizen of Windows programming at this time, it was easier
 * to use PASCAL style functions for the Windows C API than CDECL. It may have also
 * been done for space saving as CDECL does require prolog/epilog code in every function
 * call although even for 1985, I would think it relatively negiliable ..
 */

int PASCAL WinMain( hInstance, hPrevInstance, lpszCmdLine, cmdShow )
HANDLE hInstance, hPrevInstance;
LPSTR lpszCmdLine;
int cmdShow;
{
    MSG   msg;
    HWND  hWnd;
    HMENU hMenu;

    /**
     * NC: Our first real departure from Win32 is with hPrevInstance.
     * 
     * On modern Win32, this is a dummy, it's always set to NULL, but it had a purpose at one point.
     * 
     * Win16 applications can be moved and loaded/unloaded in memory. On the 8086, there is no MMU
     * to help with this process, so the Windows API has to track memory block allocations and
     * locations in real time. Memory allocation/deallocations were a major performance painpoint
     * at the time, so Windows would leave various resources loaded into the global space. Applications
     * can leave bits of themselves in memory to allow for faster load times on relaunch. Resources can
     * also be shared between applications
     * 
     * hPrevInstance is the pointer to any previous versions; NULL means that this application module
     * has never been loaded and it needs to load all it's resources into memory, and perform class
     * initialization. (see HelloInit).
     * 
     * This entire concept doesn't exist in Win32, so hPrevInstance is always set to NULL as Windows
     * applications have this modern innovation of seperate address space (something that would come
     * with the 386 processor!)
     */

    if (!hPrevInstance) {
        /* Call initialization procedure if this is the first instance */
        if (!HelloInit( hInstance ))
            return FALSE;
        }
    else {
        /* Copy data from previous instance */
        GetInstanceData( hPrevInstance, (PSTR)szAppName, 10 );
        GetInstanceData( hPrevInstance, (PSTR)szAbout, 10 );
        GetInstanceData( hPrevInstance, (PSTR)szMessage, 15 );
        GetInstanceData( hPrevInstance, (PSTR)&MessageLength, sizeof(int) );
        }

    /**
     * NC: Our next bit of weirdness is with CreateWindow.
     * 
     * CreateWindow, as the name suggests, creates a Window. The weirdness
     * is that many bits of vestigal functionality exist here, such as x/y
     * coordinates. Windows 1.0 was tiling; no window could be above or
     * below each other (Dialog boxes could, but that's a slightly different
     * beast).
     * 
     * At the time, Apple had essentially locked in the modern desktop look
     * with overlapping windows on the Lisa, and later with Macintosh.
     * Digital Research was hit with a lawsuit over this who lost. Apple
     * also later sued Microsoft over the look and feel of Windows 2.0
     * which introduced the now familar overlapping windows. Microsoft
     * eventually won in court, aruging that Apple had copied from Xerox.
     * 
     * See: Apple v. Digital Research and Apple v. Microsoft
     * 
     * The court case is kinda a messy history, but it's not that different
     * from Samsung vs. Apple and rounded corners :). Maybe a video for
     * another day
     */

    hWnd = CreateWindow((LPSTR)szAppName,
                        (LPSTR)szMessage,
                        WS_TILEDWINDOW,
                        0,    /*  x - ignored for tiled windows */
                        0,    /*  y - ignored for tiled windows */
                        0,    /* cx - ignored for tiled windows */
                        0,    /* cy - ignored for tiled windows */
                        (HWND)NULL,        /* no parent */
                        (HMENU)NULL,       /* use class menu */
                        (HANDLE)hInstance, /* handle to window instance */
                        (LPSTR)NULL        /* no params to pass on */
                        );

    /* Save instance handle for DialogBox */
    hInst = hInstance;

    /* Bind callback function with module instance */

    /**
     * NC: Ah, MakeProcInstance, I knew this was here, and it's victim of
     * one of Windows greatest programming bugs.
     * 
     * Let's get the start point here and explain a bit about Windows
     * callback model. Functions that will be called by Windows need to
     * be declared with MakeProcInstance which registers them to global
     * namespace. This works by registered DS so Windows can safely call
     * far from anywhere in the userspace (again, shared userspace).
     * 
     * For those of us who never dealt with segmented programming, let me
     * explain. The 8086 was a 16-bit processor, and could reference 64kb
     * of memory in a single swatch. The processor however supported 1 MiB
     * of memory in total, for 20-bits of address space. Memory was then
     * references through the concept of segments which were 64k windows
     * into main memory. The 8086 had four segments:
     * 
     * CS - Code Segment
     * DS - Data Segment
     * ES - Extra Segment
     * SS - Stack Segment
     * 
     * The names make their uses pretty obvious. This meant that 16-bit
     * real mode programming had two types of function calls, NEAR and FAR.
     * NEAR calls are identical to modern 32-bit programming and are for
     * functions that are within the segment or are "near".
     * 
     * FAR calls on the other hand deal with code living in another segment
     * The processor only changes CS on a FAR call and its userspace's
     * responsibility to load the other segment registers as needed. So registering
     * DS to a lookup table looks reasonable. And it would be if Microsoft actually
     * remembered how their own OS works.
     * 
     * Like a normal procedure call, far calls push the stack base pointer (bp)
     * to the stack. Obviously, SS has to get pushed along with it so the original
     * stack call be restored. So far, this is looking reasonable.
     * 
     * What if I told you that on Win16, as an invariant, SS = DS?
     * 
     * Suddenly, the above might look a lot less reasonable
     * 
     * Let me explain. If Windows tried to callback into a function that wasn't MakeProcInstance
     * or EXPORTed through the defintion file (aka dllexport), you're looking at a crash because
     * DS would be bogus. However, it would be possible to restore DS by simply deferencing bp
     * and getting the original SS. Or in short, because SS = DS, there's no need to save DS
     * for procedure calls across the global address space.
     * 
     * aka, this entire mechanism is entirely unnecessarily.
     * 
     * This was actually noted by Michael Geary, who released a utility known as FixDS who rewrote
     * function prolog/epilog code to do basically what I just described. The README for this
     * is available online is a great read: https://www.geary.com/fixds.html
     * 
     * The practical upshot is MakeProcInstance was not a thing by Windows 3.1. It still exists
     * in headers for API compatibility but is just #define-d away.
     * 
     * And now you know why this was a fail on MSFT's part. Incidentally, I love the fact I have
     * to explain an entirely obsolete mechanic (segmented programming) to explain another
     * obsolete mechanic (MakeProcInstance).
     * 
     * Writing annotations for Microsoft's Hello World from Windows 1.0 is like wading through 
     * a shared address space of pain, garbage, and leaked references.
     */

    lpprocAbout = MakeProcInstance( (FARPROC)About, hInstance );

    /**
     * NC: What's left isn't too different from "modern" Windows. We're just
     * adding About, and running the message polling loop. A note is that because
     * 16-bit Windows cooperatively multitasked, the message loop is specifically
     * what yielded time back to the OS in normal operation
     */

    /* Insert "About..." into system menu */
    hMenu = GetSystemMenu(hWnd, FALSE);
    ChangeMenu(hMenu, 0, NULL, 999, MF_APPEND | MF_SEPARATOR);
    ChangeMenu(hMenu, 0, (LPSTR)szAbout, IDSABOUT, MF_APPEND | MF_STRING);

    /* Make window visible according to the way the app is activated */
    ShowWindow( hWnd, cmdShow );
    UpdateWindow( hWnd );

    /* Polling messages from event queue */
    while (GetMessage((LPMSG)&msg, NULL, 0, 0)) {
        TranslateMessage((LPMSG)&msg);
        DispatchMessage((LPMSG)&msg);
        }

    return (int)msg.wParam;
}

/* Procedure called when the application is loaded for the first time */

/**
 * NC: I explained the basis of this back in hPrevInstance, but basically, anything
 * that is a resource (aka, in the RC file) gets loaded into global space. This is
 * where that magic happens. Class registration also happens here
 */

BOOL HelloInit( hInstance )
HANDLE hInstance;
{
    PWNDCLASS   pHelloClass;

    /* Load strings from resource */
    LoadString( hInstance, IDSNAME, (LPSTR)szAppName, 10 );
    LoadString( hInstance, IDSABOUT, (LPSTR)szAbout, 10 );
    MessageLength = LoadString( hInstance, IDSTITLE, (LPSTR)szMessage, 15 );

    /**
     * NC: Memory allocations on 16-bit Windows are a strange beast.
     * 
     * tl;dr: Windows 16-bit has the concept of Local and Global allocations, defining
     * if a memory block is used between multiple applications.
     * 
     * The long story short was Windows was designed with the idea that 16-bit programs could theoretically
     * run in their own address space. This is remarkably similar to what 286 Protected mode looks like
     * due to the fact that 286 Protected Mode is still limited to 64-kb segment size.
     * 
     * More specifically, Local allocations vs. Global told Windows what could be tossed when an application
     * went to the end. Local allocations, were only to be used with a given program, and automatically
     * got freed at the end of allocation. However, because of resource pooling, and some APIs
     * having special requirements, in practice, sometimes memory had to survive past SIGTERM unless
     * you have a crash on your hand.
     * 
     * Global Allocations survived beyond the death of an application and were a source of
     * memory leaks that required a relog. They were essentially used for program IPC and similar. Load* calls
     * were essentially global allocations that Windows itself managed the lifecycle for, and
     * could be marked SHARED (for use between multiple applications), LOCKED (must stay at physical
     * loc) and other similar features.
     * 
     * This still somewhat exists in Win32, but not in the same ad-hoc mess. COM and similar mechanisms
     * became more common in Windows for IPC and (sorta) safer in general.
     */ 

    pHelloClass = (PWNDCLASS)LocalAlloc( LPTR, sizeof(WNDCLASS) );

    pHelloClass->hCursor        = LoadCursor( NULL, IDC_ARROW );
    pHelloClass->hIcon          = LoadIcon( hInstance, MAKEINTRESOURCE(HELLOICON) );
    pHelloClass->lpszMenuName   = (LPSTR)NULL;
    pHelloClass->lpszClassName  = (LPSTR)szAppName;
    pHelloClass->hbrBackground  = (HBRUSH)GetStockObject( WHITE_BRUSH );
    pHelloClass->hInstance      = hInstance;
    pHelloClass->style          = CS_HREDRAW | CS_VREDRAW;
    pHelloClass->lpfnWndProc    = HelloWndProc;

    /**
     * NC: Class registration exists in later versions of Windows, but what it did
     * here was a bit different due to how 16-bit Windows worked
     * 
     * As stated before, everything exists in a global soup. The MSFT supplied comment
     * states that when registration of ails, Windows deallocates all memory. This is
     * partially true as I understand it. Instead, when a program quits, any bits it
     * loads stays in memory until a block is needed again. This includes code segments
     * used for windows and dialog classes (also see MakeProcInstance's rant). Class
     * registration marked these bits of code and allowed reuse. In practice, a lot
     * of things like this HELLO segment appears to just clean up its previous self
     * and re-initialize.
     *
     * Notably, class registration should always succeed. At least modern Windows
     * documentation doesn't even bother checking this.
     * 
     * https://docs.microsoft.com/en-us/windows/win32/learnwin32/creating-a-window
     *
     * Also, my understanding of this is really shaky, so this is probably inaccurate.
     * The SDK docs don't explain any of this well.
     */

    if (!RegisterClass( (LPWNDCLASS)pHelloClass ) )
        /* Initialization failed.
         * Windows will automatically deallocate all allocated memory.
         */
        return FALSE;

    LocalFree( (HANDLE)pHelloClass );
    return TRUE;        /* Initialization succeeded */
}

/**
 * NC: What follows is just the basic dialog and windows procedures.
 * These are pretty much identical to what modern Windows looks like,
 * so I've got nothing really new to add.
 */

BOOL FAR PASCAL About( hDlg, message, wParam, lParam )
HWND hDlg;
unsigned message;
WORD wParam;
LONG lParam;
{
    if (message == WM_COMMAND) {
        EndDialog( hDlg, TRUE );
        return TRUE;
        }
    else if (message == WM_INITDIALOG)
        return TRUE;
    else return FALSE;
}


void HelloPaint( hDC )
HDC hDC;
{
    TextOut( hDC,
             (short)10,
             (short)10,
             (LPSTR)szMessage,
             (short)MessageLength );
}

/* Procedures which make up the window class. */
long FAR PASCAL HelloWndProc( hWnd, message, wParam, lParam )
HWND hWnd;
unsigned message;
WORD wParam;
LONG lParam;
{
    PAINTSTRUCT ps;

    switch (message)
    {
    case WM_SYSCOMMAND:
        switch (wParam)
        {
        case IDSABOUT:
            DialogBox( hInst, MAKEINTRESOURCE(ABOUTBOX), hWnd, lpprocAbout );
            break;
        default:
            return DefWindowProc( hWnd, message, wParam, lParam );
        }
        break;

    case WM_DESTROY:
        PostQuitMessage( 0 );
        break;

    case WM_PAINT:
        BeginPaint( hWnd, (LPPAINTSTRUCT)&ps );
        HelloPaint( ps.hdc );
        EndPaint( hWnd, (LPPAINTSTRUCT)&ps );
        break;

    default:
        return DefWindowProc( hWnd, message, wParam, lParam );
        break;
    }
    return(0L);
}
