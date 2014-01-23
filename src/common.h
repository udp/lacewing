
/* vim: set et ts=3 sw=3 ft=c:
 *
 * Copyright (C) 2011, 2012, 2013 James McLaughlin.  All rights reserved.
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

#define _lacewing_internal

#ifdef _WIN32

   #if defined (_lacewing_vld)
       #include <vld.h>
   #endif

   #if defined(_DEBUG) && !defined(_lacewing_debug)
      #define _lacewing_debug
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
      #define _lacewing_android
      #include "../jni/config.h"
   #else
      #ifdef HAVE_CONFIG_H
         #include "../config.h"
      #else
         #error "Valid config.h required to build!  Run ./configure"
      #endif
   #endif

#endif

#ifdef _WIN32
   #ifndef _lacewing_static
      #define lw_import __declspec(dllexport)
   #endif
#else
   #ifdef __GNUC__
      #ifndef _lacewing_static
         #define lw_import __attribute__((visibility("default")))
      #endif
   #else
      #define lw_import
   #endif
#endif

/* For convenience, some types (such as lw_client and lw_ws_req) are typedef-d
 * to lw_stream in lacewing.h instead of to their extended structure.  The
 * typedefs here are the internal ones mapping everything to their _real_ type,
 * so that the fields after the lw_stream ones may be accessed inside the
 * library.
 */

 typedef struct _lw_thread            * lw_thread;
 typedef struct _lw_addr              * lw_addr;
 typedef struct _lw_filter            * lw_filter;
 typedef struct _lw_pump              * lw_pump;
 typedef struct _lw_pump_watch        * lw_pump_watch;
 typedef struct _lw_eventpump         * lw_eventpump;
 typedef struct _lw_stream            * lw_stream;
 typedef struct _lw_fdstream          * lw_fdstream;
 typedef struct _lw_file              * lw_file;
 typedef struct _lw_timer             * lw_timer;
 typedef struct _lw_sync              * lw_sync;
 typedef struct _lw_event             * lw_event;
 typedef struct _lw_error             * lw_error;
 typedef struct _lw_client            * lw_client;
 typedef struct _lw_server            * lw_server;
 typedef struct _lw_server_client     * lw_server_client;
 typedef struct _lw_udp               * lw_udp;
 typedef struct _lw_flashpolicy       * lw_flashpolicy;
 typedef struct _lw_ws                * lw_ws;
 typedef struct _lw_ws_req            * lw_ws_req;
 typedef struct _lw_ws_req_hdr        * lw_ws_req_hdr;
 typedef struct _lw_ws_req_param      * lw_ws_req_param;
 typedef struct _lw_ws_req_cookie     * lw_ws_req_cookie;
 typedef struct _lw_ws_upload         * lw_ws_upload;
 typedef struct _lw_ws_upload_hdr     * lw_ws_upload_hdr;
 typedef struct _lw_ws_session        * lw_ws_session;
 typedef struct _lw_ws_sessionitem    * lw_ws_sessionitem;

#include "../include/lacewing.h"

#ifdef _MSC_VER
    #ifndef __cplusplus
        #error "Can only compile as C++ with MSVC"
    #endif
    #pragma warning(disable: 4200) /* zero-sized array in struct/union */
    #pragma warning(disable: 4800) /* forcing value to bool 'true' or 'false' */
    #include "windows/typeof.h"
#endif

#include "list.h"

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

#include "refcount.h"

#ifndef container_of
   #define container_of(p, type, v) \
        ((type *)  (((char *) p) - offsetof(type, v)) )
        
#endif

#include "heapbuffer.h"

#include "../deps/uthash/uthash.h"
#include "nvhash.h"

#define lwp_max_path 512

#ifdef _WIN32
   #include "windows/common.h"
#else
   #include "unix/common.h"
#endif

#if defined(HAVE_MALLOC_H) || defined(_WIN32)
   #include <malloc.h>
#endif

#if defined(_lacewing_debug) || defined(_lacewing_debug_output)
   #define lwp_trace lw_trace
#else
   #define lwp_trace(x, ...)
#endif

/* TODO : find the optimal value for this?  make adjustable? */

#define lwp_default_buffer_size (1024 * 64)


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

void lwp_to_lowercase (char * str);

extern const char * const lwp_weekdays [];
extern const char * const lwp_months [];

time_t lwp_parse_time (const char *);

lwp_socket lwp_create_server_socket (lw_filter, int type, int protocol, lw_error);

#ifdef __cplusplus

   } /* extern "C" */

   using namespace lacewing;
   #include <new> 

#endif

#define lwp_def_hook(c, hook) \
    void lw_##c##_on_##hook (lw_##c ctx, lw_##c##_hook_##hook hook)           \
    {   ctx->on_##hook = hook;                                                \
    }                                                                         \


