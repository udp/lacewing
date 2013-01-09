
/* vim: set et ts=4 sw=4 ft=cpp:
 *
 * Copyright (C) 2011, 2012 James McLaughlin.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "../lw_common.h"

struct Thread::Internal
{
    void * Function, * Parameter;
    char * Name;

    HANDLE Thread;

    Internal (const char * _Name, void * _Function)
        : Function (_Function), Name (strdup (_Name))
    {
        Thread = INVALID_HANDLE_VALUE;
    }

    ~ Internal ()
    {
        free (Name);
    }
};

Thread::Thread (const char * Name, void * Function)
{
    internal = new Internal (Name, Function);
}

Thread::~Thread ()
{
    Join ();

    delete internal;
}

static int ThreadWrapper (Thread::Internal * internal)
{
    #ifdef _MSC_VER

        struct
        {
              DWORD dwType;
              LPCSTR szName;
              DWORD dwThreadID;
              DWORD dwFlags;

        } ThreadNameInfo;

        ThreadNameInfo.dwFlags     = 0;
        ThreadNameInfo.dwType      = 0x1000;
        ThreadNameInfo.szName      = internal->Name;
        ThreadNameInfo.dwThreadID  = -1;

        __try
        {   RaiseException (0x406D1388, 0, sizeof (ThreadNameInfo) / sizeof (ULONG), (ULONG_PTR *) &ThreadNameInfo);
        }
        __except (EXCEPTION_CONTINUE_EXECUTION)
        {
        }

    #endif

    return ((int (*) (void *)) internal->Function) (internal->Parameter);
}

void Thread::Start (void * Parameter)
{
    if (Started ())
        return;

    internal->Parameter = Parameter;
    
    internal->Thread = (HANDLE) _beginthreadex (0, 0,
            (unsigned (__stdcall *) (void *)) ThreadWrapper, internal, 0, 0);
}

bool Thread::Started ()
{
    return internal->Thread != INVALID_HANDLE_VALUE;
}

int Thread::Join ()
{
    if (!Started ())
        return -1;

    DWORD ExitCode = -1;

    if (WaitForSingleObject (internal->Thread, INFINITE) == WAIT_OBJECT_0)
        GetExitCodeThread (internal->Thread, &ExitCode);

    return (int) (lw_iptr) ExitCode;
}


