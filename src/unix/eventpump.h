
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

#include "../pump.h"

#define max_events  16

struct _lw_pump_watch
{
   lw_pump_callback on_read_ready, on_write_ready;
   lw_bool edge_triggered;

   int fd;
   void * tag;
};

struct _lw_eventpump
{  
   struct _lw_pump pump;

   int queue;

   lw_sync sync_signals;

   int signalpipe_read, signalpipe_write;
   list (void *, signalparams);

   /* for start_sleepy_ticking
    */
   struct
   {
      lw_thread thread;

      int num_events;

      #if defined (_lacewing_use_epoll)
        struct epoll_event events [max_events];
      #elif defined (_lacewing_use_kqueue)
        struct kevent events [max_events];
      #endif

      lw_event resume_event;

   } watcher;

   void (lw_callback * on_tick_needed) (lw_eventpump);
};

const lw_pumpdef def_eventpump;


