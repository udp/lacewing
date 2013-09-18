
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

#include "../common.h"
#include "eventpump.h"

/* TODO: there is a lot of repeated if-epoll-this-elif-kqueue-that in
 * this file.  It would be better to abstract over epoll/kqueue once and use
 * that API here.
 */

enum
{
   sig_exit_eventloop,
   sig_remove,
   sig_post
};

static void watcher (lw_eventpump ctx);

lw_eventpump lw_eventpump_new ()
{
   lw_eventpump ctx = calloc (sizeof (*ctx), 1);

   if (!ctx)
      return 0;

   ctx->watcher.thread = lw_thread_new ("watcher", (void *) watcher);
   ctx->watcher.resume_event = lw_event_new ();

   lwp_pump_init (&ctx->pump, &def_eventpump);

   ctx->sync_signals = lw_sync_new ();

   int signalpipe [2];
   pipe (signalpipe);

   ctx->signalpipe_read   = signalpipe [0];
   ctx->signalpipe_write  = signalpipe [1];

   fcntl (ctx->signalpipe_read, F_SETFL,
         fcntl (ctx->signalpipe_read, F_GETFL, 0) | O_NONBLOCK);

   #if defined (_lacewing_use_epoll)

      ctx->queue = epoll_create (32);

      struct epoll_event event =
      {
         .events = EPOLLIN | EPOLLET
      };

      epoll_ctl (ctx->queue, EPOLL_CTL_ADD, ctx->signalpipe_read, &event);

   #elif defined (_lacewing_use_kqueue)

      ctx->queue = kqueue ();

      struct kevent event;

      EV_SET (&event, ctx->signalpipe_read, EVFILT_READ,
            EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, 0);

      kevent (ctx->queue, &event, 1, 0, 0, 0);

   #endif

   return ctx;
}

static void def_cleanup (lw_pump pump)
{
   lw_eventpump ctx = (lw_eventpump) pump;

   close (ctx->queue);
   ctx->queue = -1;
   
   if (lw_thread_started (ctx->watcher.thread))
   {
      lw_event_signal (ctx->watcher.resume_event);
      lw_thread_join (ctx->watcher.thread);
   }

   lw_event_delete (ctx->watcher.resume_event);

   /* TODO */
}

lw_bool ready (lw_eventpump ctx, lw_pump_watch watch,
               lw_bool read_ready, lw_bool write_ready)
{
   if (watch)
   {
      if (read_ready && watch->on_read_ready)
         watch->on_read_ready (watch->tag);

      if (write_ready && watch->on_write_ready)
         watch->on_write_ready (watch->tag);

      return lw_true;
   }

   /* A null event means it must be the signal pipe */

   lw_sync_lock (ctx->sync_signals);

   char signal;

   if (read (ctx->signalpipe_read, &signal, sizeof (signal)) == -1)
   {
      lw_sync_release (ctx->sync_signals);
      return lw_true;
   }

   switch (signal)
   {
      case sig_exit_eventloop:
      {
         lw_sync_release (ctx->sync_signals);
         return lw_false;
      }

      case sig_remove:
      {
         lw_pump_watch to_remove = list_front (ctx->signalparams);
         list_pop_front (ctx->signalparams);

         free (to_remove);

         lw_pump_remove_user ((lw_pump) ctx);

         break;
      }

      case sig_post:
      {
         void * func = list_front (ctx->signalparams);
         list_pop_front (ctx->signalparams);

         void * param = list_front (ctx->signalparams);
         list_pop_front (ctx->signalparams);

         ((void * (*) (void *)) func) (param);

         break;
      }
   };

   lw_sync_release (ctx->sync_signals);

   return lw_true;
}

/* process_event: interprets an epoll_event or kevent and calls ready()
 */
#if defined (_lacewing_use_epoll)
   static lw_bool process_event (lw_eventpump ctx, struct epoll_event event)
   {
      return ready
      (
         ctx,

         (lw_pump_watch) event.data.ptr,

         (event.events & EPOLLIN) != 0
          || (event.events & EPOLLHUP) != 0
          || (event.events & EPOLLRDHUP) != 0,

         (event.events & EPOLLOUT) != 0
      );
   }
#else
   static lw_bool process_event (lw_eventpump ctx, struct kevent event)
   {
      if (event.filter == EVFILT_TIMER)
      {
         lw_timer_force_tick ((lw_timer) event.udata);

         return lw_true;
      }
      else
      {
         return ready
         (
            ctx, 

            (lw_pump_watch) event.udata,

            event.filter == EVFILT_READ || (event.flags & EV_EOF),
            event.filter == EVFILT_WRITE
         );
      }
   }
#endif

lw_error lw_eventpump_tick (lw_eventpump ctx)
{
   lw_bool need_watcher_resume = lw_false;
    
   if (ctx->watcher.num_events > 0)
   {
      /* sleepy ticking: the watcher thread already grabbed some events we
       * need to process.
       */
      for (int i = 0; i < ctx->watcher.num_events; ++ i)
         process_event (ctx, ctx->watcher.events [i]);

      ctx->watcher.num_events = 0;
       
      need_watcher_resume = lw_true;
   }

   #if defined (_lacewing_use_epoll)

      struct epoll_event epoll_events [max_events];
      int count = epoll_wait (ctx->queue, epoll_events, max_events, 0);

      for (int i = 0; i < count; ++ i)
         process_event (ctx, epoll_events [i]);
      
   #elif defined (_lacewing_use_kqueue)
       
      struct kevent kevents [max_events];
   
      struct timespec zero = { 0 };
   
      int count = kevent (ctx->queue, 0, 0, kevents, max_events, &zero);
   
      for (int i = 0; i < count; ++ i)
         process_event (ctx, kevents [i]);
   
   #endif
    
   if (need_watcher_resume)
      lw_event_signal (ctx->watcher.resume_event);
    
   return 0;
}

lw_error lw_eventpump_start_eventloop (lw_eventpump ctx)
{
   int do_loop = 1;
   while (do_loop)
   {
      #if defined (_lacewing_use_epoll)

         struct epoll_event epoll_events [max_events];
         int count = epoll_wait (ctx->queue, epoll_events, max_events, -1);
      
         if (count == -1)
         {
            if (errno == EINTR)
               continue;

            lwp_trace ("epoll error: %d", errno);
            break;
         }
   
         for (int i = 0; i < count; ++ i)
            if (!process_event (ctx, epoll_events [i]))
            {
               do_loop = 0;
               break;
            }
         
      #elif defined (_lacewing_use_kqueue)
          
         struct kevent kevents [max_events];
      
         int count = kevent (ctx->queue, 0, 0, kevents, max_events, 0);
      
         if (count == -1)
         {
            if (errno == EINTR)
               continue;

            lwp_trace ("kevent error: %d", errno);
            break;
         }

         for (int i = 0; i < count; ++ i)
            if (!process_event (ctx, kevents [i]))
            {
               do_loop = 0;
               break;
            }
      
      #endif
   }

   return 0;
}

void lw_eventpump_post_eventloop_exit (lw_eventpump ctx)
{
   lw_sync_lock (ctx->sync_signals);

      char signal = sig_exit_eventloop;
      write (ctx->signalpipe_write, &signal, sizeof (signal));

   lw_sync_release (ctx->sync_signals);
}

lw_error lw_eventpump_start_sleepy_ticking
    (lw_eventpump ctx, void (lw_callback * on_tick_needed) (lw_eventpump))
{
   ctx->on_tick_needed = on_tick_needed;    
   lw_thread_start (ctx->watcher.thread, ctx);

   return 0;
}

static void watcher (lw_eventpump ctx)
{
   for (;;)
   {
      assert (ctx->watcher.num_events == 0);

      int count;

      #if defined (_lacewing_use_epoll)

         count = epoll_wait (ctx->queue, ctx->watcher.events, max_events, -1);

         if (count == -1)
         {
            if (errno == EINTR)
               continue;

            lwp_trace ("epoll error: %d", errno);
            break;
         }
         
      #elif defined (_lacewing_use_kqueue)
          
         struct kevent kevents [max_events];
      
         count = kevent
            (ctx->queue, 0, 0, ctx->watcher.events, max_events, 0);
      
         if (count == -1)
         {
            if (errno == EINTR)
               continue;

            lwp_trace ("kevent error: %d", errno);
            break;
         }

      #endif

      if (count == 0)
         continue;

      ctx->watcher.num_events = count;

      /* We have some events.  Notify the application from this thread, then
       * wait for tick() to be called from the application main thread.
       */
      ctx->on_tick_needed (ctx);

      lw_event_wait (ctx->watcher.resume_event, -1);
      lw_event_unsignal (ctx->watcher.resume_event);
   }
}

static lw_pump_watch def_add (lw_pump pump, int fd, void * tag,
                              lw_pump_callback on_read_ready,
                              lw_pump_callback on_write_ready,
                              lw_bool edge_triggered)
{
   lw_eventpump ctx = (lw_eventpump) pump;

   if ((!on_read_ready) && (!on_write_ready))
      return 0;

   lw_pump_watch watch = calloc (sizeof (*watch), 1);

   if (!watch)
      return 0;

   watch->fd = fd;

   #if defined (_lacewing_use_epoll)
    
      watch->on_read_ready = on_read_ready;
      watch->on_write_ready = on_write_ready;
      watch->edge_triggered = edge_triggered;
      watch->tag = tag;

      struct epoll_event event;
      memset (&event, 0, sizeof (event));

      event.data.ptr = watch;
        
      event.events = (on_read_ready ? EPOLLIN : 0) |
                        (on_write_ready ? EPOLLOUT : 0) |
                           (edge_triggered ? EPOLLET : 0);

      epoll_ctl (ctx->queue, EPOLL_CTL_ADD, fd, &event);

  #elif defined (_lacewing_use_kqueue)

      lw_pump_update_callbacks (&ctx->pump, watch, tag,
                                on_read_ready, on_write_ready,
                                edge_triggered);

  #endif

  lw_pump_add_user ((lw_pump) ctx);

  return watch;
}

static void def_update_callbacks (lw_pump pump,
                                  lw_pump_watch watch, void * tag,
                                  lw_pump_callback on_read_ready,
                                  lw_pump_callback on_write_ready,
                                  lw_bool edge_triggered)
{
   lw_eventpump ctx = (lw_eventpump) pump;

   if ( ((on_read_ready != 0) != (watch->on_read_ready != 0))
         || ((on_write_ready != 0) != (watch->on_write_ready != 0))
         || (edge_triggered != watch->edge_triggered) )
   {
      #if defined (_lacewing_use_epoll)

         struct epoll_event event;
         memset (&event, 0, sizeof (event));

         event.data.ptr = watch;

         event.events = (on_read_ready ? EPOLLIN : 0) |
                          (on_write_ready ? EPOLLOUT : 0) |
                             (edge_triggered ? EPOLLET : 0);

         epoll_ctl (ctx->queue, EPOLL_CTL_MOD, watch->fd, &event);

      #elif defined (_lacewing_use_kqueue)
     
         struct kevent change;

         if (on_read_ready)
         {
             EV_SET (&change, watch->fd, EVFILT_READ,
                         EV_ADD | EV_ENABLE | EV_EOF |
                            (edge_triggered ? EV_CLEAR : 0), 0, 0, watch);

             kevent (ctx->queue, &change, 1, 0, 0, 0);
         }

         if (on_write_ready)
         {
             EV_SET (&change, watch->fd, EVFILT_WRITE,
                         EV_ADD | EV_ENABLE | EV_EOF |
                            (edge_triggered ? EV_CLEAR : 0), 0, 0, watch);

             kevent (ctx->queue, &change, 1, 0, 0, 0);
         }

      #endif
   }

   watch->on_read_ready = on_read_ready;
   watch->on_write_ready = on_write_ready;
   watch->edge_triggered = edge_triggered;
   watch->tag = tag;
}

static void def_remove (lw_pump pump, lw_pump_watch watch)
{
   lw_eventpump ctx = (lw_eventpump) pump;

   /* TODO : Should this remove the FD from epoll/kqueue immediately? */

   watch->on_read_ready = 0;
   watch->on_write_ready = 0;

   lw_sync_lock (ctx->sync_signals);

      list_push (ctx->signalparams, watch);

      char signal = sig_remove;
      write (ctx->signalpipe_write, &signal, sizeof (signal));

   lw_sync_release (ctx->sync_signals);
}

static void def_post (lw_pump pump, void * func, void * param)
{
   lw_eventpump ctx = (lw_eventpump) pump;

   lw_sync_lock (ctx->sync_signals);

      list_push (ctx->signalparams, func);
      list_push (ctx->signalparams, param);

      char signal = sig_post;
      write (ctx->signalpipe_write, &signal, sizeof (signal));

   lw_sync_release (ctx->sync_signals);
}

const lw_pumpdef def_eventpump =
{
   .add               = def_add,
   .remove            = def_remove,
   .update_callbacks  = def_update_callbacks,
   .post              = def_post,
   .cleanup           = def_cleanup
};

