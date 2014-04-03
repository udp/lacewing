
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

#include "../../common.h"
#include "eventqueue.h"

int lwp_eventqueue_new ()
{
   return kqueue ();
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
   lwp_eventqueue_update (queue, fd,
                          lw_false, read,
                          lw_false, write,
                          lw_false, edge_triggered,
                          NULL, tag);
}

void lwp_eventqueue_update (lwp_eventqueue queue,
                            int fd,
                            lw_bool was_reading, lw_bool read,
                            lw_bool was_writing, lw_bool write,
                            lw_bool was_edge_triggered, lw_bool edge_triggered,
                            void * old_tag, void * tag)
{
   struct kevent changes [2];
   int num_changes = 0;

   if (read)
   {
      struct kevent * change = changes + (num_changes ++);

      EV_SET (change, fd, EVFILT_READ,
                 EV_ADD | EV_ENABLE | EV_EOF |
                 (edge_triggered ? EV_CLEAR : 0), 0, 0, tag);
   }
   else
   {
      if (was_reading)
      {
         /* Was reading; stop now
          */
         struct kevent * change = changes + (num_changes ++);

         EV_SET (change, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
      }
   }

   if (write)
   {
      struct kevent * change = changes + (num_changes ++);

      EV_SET (change, fd, EVFILT_WRITE,
                 EV_ADD | EV_ENABLE | EV_EOF |
                 (edge_triggered ? EV_CLEAR : 0), 0, 0, tag);
   }
   else
   {
      if (was_writing)
      {
         /* Was writing; stop now
          */
         struct kevent * change = changes + (num_changes ++);

         EV_SET (change, fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
      }
   }

   kevent (queue, changes, num_changes, 0, 0, 0);
}

int lwp_eventqueue_drain (lwp_eventqueue queue,
                          lw_bool block,
                          int max_events,
                          lwp_eventqueue_event * events)
{
   struct timespec zero = {};
   struct timespec * timeout = NULL;

   if (!block)
      timeout = &zero;

   return kevent (queue, 0, 0, events, max_events, timeout);
}

lw_bool lwp_eventqueue_event_read_ready (lwp_eventqueue_event event)
{
   return event.filter == EVFILT_READ || (event.flags & EV_EOF);
}

lw_bool lwp_eventqueue_event_write_ready (lwp_eventqueue_event event)
{
   return event.filter == EVFILT_WRITE;
}

void * lwp_eventqueue_event_tag (lwp_eventqueue_event event)
{
   return event.udata;
}


