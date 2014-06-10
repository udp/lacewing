
/* vim: set et ts=3 sw=3 ft=c:
 *
 * Copyright (C) 2011, 2012, 2013 James McLaughlin et al.  All rights reserved.
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

#include <sys/types.h> 
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/utsname.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sched.h>

#ifdef ANDROID
   #include <time64.h>
   #include <android/log.h>
#endif

#ifdef HAVE_SYS_TIMERFD_H
   #include <sys/timerfd.h>
   
   #ifndef USE_KQUEUE
      #define _lacewing_use_timerfd
   #endif
#endif

#ifdef HAVE_SYS_SENDFILE_H
   #include <sys/sendfile.h>
#endif

#ifdef HAVE_NETDB_H
   #include <netdb.h>
#endif

#ifndef __APPLE__
   #ifdef TCP_CORK
      #define lw_cork TCP_CORK
   #else
      #ifdef TCP_NOPUSH
         #define lw_cork TCP_NOPUSH 
      #endif
   #endif
#endif

#if defined(USE_EPOLL)
   #include <sys/epoll.h>

   #ifndef EPOLLRDHUP
      #define EPOLLRDHUP 0x2000
   #endif
#elif defined(USE_KQUEUE)
   #include <sys/event.h>
#endif

#include <string.h>
#include <limits.h>

#ifdef HAVE_SYS_PRCTL_H
   #include <sys/prctl.h>
#endif

#ifdef ENABLE_SSL
   #include <openssl/ssl.h>
   #include <openssl/md5.h>
   #include <openssl/sha.h>
   #include <openssl/err.h>
#endif

#ifdef OPENSSL_NPN_NEGOTIATED
   #define _lacewing_npn
#endif

#define lwp_last_error errno
#define lwp_last_socket_error errno

#define _atoi64 atoll

typedef int lwp_socket;

#define lwp_vsnprintf vsnprintf
#define lwp_snprintf snprintf
#define lwp_fmt_size "%zd"

