
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

/* TODO: this is non-portable
 */
static fd_set fdset_remove (fd_set a, fd_set b)
{
   fd_set res;

   for (int i = 0; i < sizeof (fd_set); ++ i)
      ((char *) &res) [i] = ((char *) &a) [i] & ~ ((char *) &b) [i];
   
   return res;
}

#define event_flag_read_ready  (1 << 0)
#define event_flag_write_ready  (1 << 1)

struct _lwp_eventqueue
{
   fd_set read_set, write_set,
          read_set_LT, write_set_LT,  /* level-triggered */
          read_ready_set, write_ready_set;

   int max_fd;

   void * tags [FD_SETSIZE];
};

lwp_eventqueue lwp_eventqueue_new ()
{
   lwp_eventqueue queue = calloc (sizeof (*queue), 1);

   if (!queue)
      return NULL;

   return queue;
}

void lwp_eventqueue_delete (lwp_eventqueue queue)
{
   free (queue);
}

void lwp_eventqueue_add (lwp_eventqueue queue,
                         int fd,
                         lw_bool read,
                         lw_bool write,
                         lw_bool edge_triggered,
                         void * tag)
{
   if (fd >= queue->max_fd)
      queue->max_fd = fd + 1;

   lwp_eventqueue_update (queue,
                          fd,
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
   if (read)
   {
      FD_SET (fd, &queue->read_set);

      if (!edge_triggered)
      {
         FD_SET (fd, &queue->read_set_LT);
      }
      else
      {
         if (!was_edge_triggered)
            FD_CLR (fd, &queue->read_set_LT);
      }
   }
   else
   {
      FD_CLR (fd, &queue->read_set);
      FD_CLR (fd, &queue->read_set_LT);
   }

   if (write)
   {
      FD_SET (fd, &queue->write_set);

      if (!edge_triggered)
      {
         FD_SET (fd, &queue->write_set_LT);
      }
      else
      {
         if (!was_edge_triggered)
            FD_CLR (fd, &queue->write_set_LT);
      }
   }
   else
   {
      FD_CLR (fd, &queue->write_set);
      FD_CLR (fd, &queue->write_set_LT);
   }

   queue->tags [fd] = tag;
}

int lwp_eventqueue_drain (lwp_eventqueue queue,
                          lw_bool block,
                          int max_events,
                          lwp_eventqueue_event * events)
{
   struct timeval zero_timeout = {};

   /* First we need to update the set of ready FDs, as some of those FDs may
    * not be ready any longer.
    */
   int result = select (queue->max_fd,
                        &queue->read_ready_set, &queue->write_ready_set, NULL,
                        &zero_timeout); 
   
   if (result == -1)
   {
      lwp_trace ("select() error: %d", errno);
      return 0;
   }

   /* We only want to select on the FDs that aren't already known to be ready
    */
   fd_set read_set = fdset_remove (queue->read_set, queue->read_ready_set);
   fd_set write_set = fdset_remove (queue->write_set, queue->write_ready_set);

   struct timeval * timeout = NULL;

   if (!block)
      timeout = &zero_timeout;

   result = select (queue->max_fd,
                    &read_set, &write_set, NULL,
                    timeout);

   if (result == -1)
   {
      lwp_trace ("select() error: %d", errno);
      return 0;
   }

   /* Now generate the list of events.
    */
   int num_events = 0;

   for (int fd = 0; fd < queue->max_fd; ++ fd)
   {
      lw_i8 flags = 0;

      if (FD_ISSET (fd, &read_set))
      {
         flags |= event_flag_read_ready;

         if (!FD_ISSET (fd, &queue->read_set_LT))
            FD_SET (fd, &queue->read_ready_set);
      }

      if (FD_ISSET (fd, &write_set))
      {
         flags |= event_flag_write_ready;

         if (!FD_ISSET (fd, &queue->write_set_LT))
            FD_SET (fd, &queue->write_ready_set);
      }

      if (!flags)
         continue;

      lwp_eventqueue_event * event = events + (num_events ++);

      event->flags = flags;
      event->tag = queue->tags [fd];
   }

   return num_events;
}

lw_bool lwp_eventqueue_event_read_ready (lwp_eventqueue_event event)
{
   return event.flags & event_flag_read_ready;
}

lw_bool lwp_eventqueue_event_write_ready (lwp_eventqueue_event event)
{
   return event.flags & event_flag_write_ready;
}

void * lwp_eventqueue_event_tag (lwp_eventqueue_event event)
{
   return event.tag;
}


