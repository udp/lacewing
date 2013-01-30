
/* vim: set et ts=3 sw=3 ft=c:
 *
 * Copyright (C) 2011, 2012 James McLaughlin et al.  All rights reserved.
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

#ifndef WINVER
   #define WINVER 0x0501
#endif

#ifndef _WIN32_WINNT
   #define _WIN32_WINNT 0x0501
#endif

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <mswsock.h>

#define SECURITY_WIN32

#include <security.h>
#include <sspi.h>
#include <wincrypt.h>
#include <schannel.h>
#include <process.h>

#include "lw_compat.h"

#ifdef _MSC_VER
    #define strcasecmp stricmp
    typedef SSIZE_T ssize_t;
#endif

typedef SOCKET lwp_socket;

#define lwp_vsnprintf _vsnprintf
#define lwp_snprintf _snprintf
#define lwp_fmt_size "%Id"

#define lwp_last_error GetLastError()
#define lwp_last_socket_error WSAGetLastError()

WCHAR * utf8_to_wchar (const char * str);