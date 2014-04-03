
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

#include "../common.h"
#include "eventqueue.h"

int lwp_eventqueue_new ()
{
   return epoll_create (32);
}

void lwp_eventqueue_delete (lwp_eventqueue queue)
{
   close (queue);
}

void lwp_eventqueue_add (lwp_eventqueue queue,
                         int fd,
                         lw_bool read,
                         lw_bool write,
                        lw_bool edge_triggered,
                         void * tag)
{
   struct epoll_event event = {};

   event.data.ptr = watch;

   event.events = (read ? EPOLLIN : 0) |
                  (write ? EPOLLOUT : 0) |
                  (edge_triggered ? EPOLLET : 0);

   epoll_ctl (queue, EPOLL_CTL_ADD, fd, &event);
}

void lwp_eventqueue_update (lwp_eventqueue,
                            int fd,
                            lw_bool was_reading, lw_bool read,
                            lw_bool was_writing, lw_bool write,
                            lw_bool was_edge_triggered, lw_bool edge_triggered,
                            void * old_tag, void * tag);
{
   struct epoll_event event = {};

   event.data.ptr = tag;

   if (read || write)
   {
      event.events = (read ? EPOLLIN : 0) |
                     (write ? EPOLLOUT : 0) |
                     (edge_triggered ? EPOLLET : 0);

      epoll_ctl (queue, EPOLL_CTL_MOD, fd, &event);
   }
   else
   {
      epoll_ctl (queue, EPOLL_CTL_DEL, fd, &event);
   }
}

lw_bool lwp_eventqueue_event_read_ready (lwp_eventqueue_event event)
{
   return event.events & (EPOLLIN | EPOLLHUP | EPOLLRDHUP);
}

lw_bool lwp_eventqueue_event_write_ready (lwp_eventqueue_event event)
{
   return event.events & EPOLLOUT;
}

void * lwp_eventqueue_event_tag (lwp_eventqueue_event event)
{
   return event.data.ptr;
}

