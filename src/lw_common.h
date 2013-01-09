
/* vim: set et ts=3 sw=3 ft=c:
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

#ifdef _MSC_VER
   #pragma warning (disable : 4355)
   #pragma warning (disable : 4800)
#endif

#ifdef _WIN32

   #ifdef _DEBUG
      #define LacewingDebug
   #endif

   #ifdef _WIN64
      #define Lacewing64
   #endif

   #ifndef _CRT_SECURE_NO_WARNINGS
      #define _CRT_SECURE_NO_WARNINGS
   #endif

   #ifndef _CRT_NONSTDC_NO_WARNINGS
      #define _CRT_NONSTDC_NO_WARNINGS
   #endif

   #ifdef HAVE_CONFIG_H
      #include "../config.h"
   #endif

#else

   #ifndef _GNU_SOURCE
      #define _GNU_SOURCE
   #endif

   #ifdef ANDROID
      #define LacewingAndroid
      #include "../jni/config.h"
   #else
      #ifdef HAVE_CONFIG_H
         #include "../config.h"
      #else
         #error "Valid config.h required for non-MSVC!  Run ./configure"
      #endif
   #endif

#endif

#ifdef LacewingLibrary
   #ifdef _WIN32
      #define LacewingFunction __declspec(dllexport)
   #else
      #ifdef __GNUC__
         #define LacewingFunction __attribute__((visibility("default")))
      #endif
   #endif
#else
   #define LacewingFunction
#endif

#include "../include/lacewing.h"

#ifdef __cplusplus
   extern "C" {
#endif

void lwp_init ();

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>

#include "../deps/uthash/uthash.h"
#include "lw_nvhash.h"

#define lwp_max_path 512

#ifdef _WIN32
   #include "windows/lw_common.h"
   #undef SLIST_ENTRY
#else
   #include "unix/lw_common.h"
#endif

#include "../deps/queue.h"

#if defined(HAVE_MALLOC_H) || defined(_WIN32)
   #include <malloc.h>
#endif

#if defined(LacewingDebug) || defined(LacewingForceDebugOutput)
   void lwp_trace (const char * format, ...);
#else
   #define lwp_trace(x, ...)
#endif

/* TODO : find the optimal value for this?  make adjustable? */

const static size_t lwp_default_buffer_size = 1024 * 64; 


void lwp_disable_ipv6_only (lwp_socket socket);

struct sockaddr_storage lwp_socket_addr (lwp_socket socket);

long lwp_socket_port (lwp_socket socket);

void lwp_close_socket (lwp_socket socket);

lw_bool lwp_urldecode (const char * in, size_t in_length,
                       char * out, size_t out_length, lw_bool plus_spaces);

lw_bool lwp_begins_with (const char * string, const char * substring);

void lwp_copy_string (char * dest, const char * source, size_t size);

lw_bool lwp_find_char (const char ** str, size_t * len, char c);

ssize_t lwp_format (char ** output, const char * format, va_list args);

extern const char * const lwp_weekdays [];
extern const char * const lwp_months [];

time_t lwp_parse_time (const char *);

#ifdef __cplusplus
    } /* extern "C" */

   using namespace Lacewing;

   #include <new> 
   #include "List.h"

   lwp_socket lwp_create_server_socket
      (Lacewing::Filter &, int Type, int Protocol, Lacewing::Error &Error);

#endif

#define AutoHandlerFunctions(Public, HandlerName)                             \
   void Public::on##HandlerName (Public::Handler##HandlerName Handler)        \
   {   internal->Handlers.HandlerName = Handler;                              \
   }                                                                          \

#define AutoHandlerFlat(real_class, type, flat, handler_upper, handler_lower) \
   void flat##_on##handler_lower                                              \
         (type * _flat, flat##_handler_##handler_lower _handler)              \
   {                                                                          \
      ((real_class *) _flat)->on##handler_upper                               \
         ((real_class::Handler##handler_upper) _handler);                     \
   }                                                                          \

