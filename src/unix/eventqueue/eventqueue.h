
/* vim: set et ts=3 sw=3 sts=3 ft=c:
 *
 * Copyright (C) 2013 James McLaughlin.  All rights reserved.
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

#ifdef _lacewing_use_epoll
   #include <sys/epoll.h>

   #if !HAVE_DECL_EPOLLRDHUP
      #define EPOLLRDHUP 0x2000
   #endif
#endif

#ifdef _lacewing_use_kqueue
   #include <sys/event.h>
#endif

#if defined(_lacewing_use_kqueue)

   /* kqueue: lwp_eventqueue is a kqueue fd, _event is a kevent
    */
   typedef int lwp_eventqueue;
   typedef struct kevent lwp_eventqueue_event;

#elif defined(_lacewing_use_epoll)

   /* epoll: lwp_eventqueue is an epoll fd, _event is an epoll_event
    */
   typedef int lwp_eventqueue;
   typedef struct epoll_event lwp_eventqueue_event;

#else

   /* select or other custom eventqueue implementation
    */
   typedef struct _lwp_eventqueue * lwp_eventqueue;

   typedef struct _lwp_eventqueue_event
   {
      lw_i8 flags;
      void * tag;

   } lwp_eventqueue_event;

#endif

lwp_eventqueue lwp_eventqueue_new ();
void lwp_eventqueue_delete (lwp_eventqueue);


/* add: add a file descriptor to the eventqueue
 */
void lwp_eventqueue_add (lwp_eventqueue,
                         int fd,
                         lw_bool read,
                         lw_bool write,
                         lw_bool edge_triggered,
                         void * tag);


/* update: update an existing watched file descriptor
 */
void lwp_eventqueue_update (lwp_eventqueue,
                            int fd,
                            lw_bool was_reading, lw_bool read,
                            lw_bool was_writing, lw_bool write,
                            lw_bool was_edge_triggered, lw_bool edge_triggered,
                            void * old_tag, void * new_tag);

/* drain: drain pending events from the queue
 */
int lwp_eventqueue_drain (lwp_eventqueue,
                          lw_bool block,
                          int max_events,
                          lwp_eventqueue_event * events);

lw_bool lwp_eventqueue_event_read_ready (lwp_eventqueue_event);
lw_bool lwp_eventqueue_event_write_ready (lwp_eventqueue_event);

void * lwp_eventqueue_event_tag (lwp_eventqueue_event);



