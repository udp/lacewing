
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

enum
{
   sig_exit_eventloop,
   sig_remove,
   sig_post
};

#ifdef ENABLE_THREADS
   static void watcher (lw_eventpump ctx);
#endif

lw_eventpump lw_eventpump_new ()
{
   lw_eventpump ctx = calloc (sizeof (*ctx), 1);

   if (!ctx)
      return NULL;

   #ifdef ENABLE_THREADS
      ctx->watcher.thread = lw_thread_new ("watcher", (void *) watcher);
      ctx->watcher.resume_event = lw_event_new ();
   #endif

   lwp_pump_init (&ctx->pump, &def_eventpump);

   ctx->sync_signals = lw_sync_new ();

   int signalpipe [2];
   pipe (signalpipe);

   ctx->signalpipe_read   = signalpipe [0];
   ctx->signalpipe_write  = signalpipe [1];

   fcntl (ctx->signalpipe_read, F_SETFL,
         fcntl (ctx->signalpipe_read, F_GETFL, 0) | O_NONBLOCK);

   ctx->queue = lwp_eventqueue_new ();

   lwp_eventqueue_add (ctx->queue, ctx->signalpipe_read,
                       lw_true, lw_false, lw_true,
                       NULL);

   return ctx;
}

static void def_cleanup (lw_pump pump)
{
   lw_eventpump ctx = (lw_eventpump) pump;

   lwp_eventqueue_delete (ctx->queue);
   
   #ifdef ENABLE_THREADS

      if (lw_thread_started (ctx->watcher.thread))
      {
         lw_event_signal (ctx->watcher.resume_event);
         lw_thread_join (ctx->watcher.thread);
      }

      lw_event_delete (ctx->watcher.resume_event);

   #endif

   /* TODO */
}

lw_bool process_event (lw_eventpump ctx, lwp_eventqueue_event event)
{
   lw_bool read_ready = lwp_eventqueue_event_read_ready (event),
           write_ready = lwp_eventqueue_event_write_ready (event);

   lw_pump_watch watch = lwp_eventqueue_event_tag (event);

   /* fudge: nothing kqueue specific belongs in this file, but since the
    * kqueue code doesn't actually look at the events it's the only place
    * we can put it.
    */
   #ifdef lacewing_use_kqueue

      if (event.filter == EVFILT_TIMER)
      {
         lw_timer_force_tick ((lw_timer) event.udata);
         return lw_true;
      }

   #endif

   if (watch)
   {
      if (read_ready && watch->on_read_ready)
         watch->on_read_ready (watch->tag);

      if (write_ready && watch->on_write_ready)
         watch->on_write_ready (watch->tag);

      return lw_true;
   }

   /* A null tag means it must be the signal pipe */

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

lw_error lw_eventpump_tick (lw_eventpump ctx)
{
   lw_bool need_watcher_resume = lw_false;

   #ifdef ENABLE_THREADS

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

   #endif

   lwp_eventqueue_event events [max_events];

   int count = lwp_eventqueue_drain (ctx->queue, lw_false, max_events, events);

   for (int i = 0; i < count; ++ i)
      process_event (ctx, events [i]);
   
   #ifdef ENABLE_THREADS
      if (need_watcher_resume)
         lw_event_signal (ctx->watcher.resume_event);
   #endif

   return 0;
}

lw_error lw_eventpump_start_eventloop (lw_eventpump ctx)
{
   int do_loop = 1;

   while (do_loop)
   {
      lwp_eventqueue_event events [max_events];

      int count = lwp_eventqueue_drain (ctx->queue, lw_true, max_events, events);

      if (count == -1)
      {
         if (errno == EINTR)
            continue;

         lwp_trace ("epoll error: %d", errno);
         break;
      }

      for (int i = 0; i < count; ++ i)
      {
         if (!process_event (ctx, events [i]))
         {
            do_loop = 0;
            break;
         }
      }
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
   #ifdef ENABLE_THREADS
      ctx->on_tick_needed = on_tick_needed;    
      lw_thread_start (ctx->watcher.thread, ctx);
   #else
      /* TODO error */
   #endif

   return 0;
}

#ifdef ENABLE_THREADS

static void watcher (lw_eventpump ctx)
{
   for (;;)
   {
      assert (ctx->watcher.num_events == 0);

      int count = lwp_eventqueue_drain (ctx->queue,
                                        lw_true,
                                        max_events,
                                        ctx->watcher.events);

      if (count == -1)
      {
         if (errno == EINTR)
            continue;

         lwp_trace ("drain error: %d", errno);
         break;
      }
         
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

#endif

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
   watch->on_read_ready = on_read_ready;
   watch->on_write_ready = on_write_ready;
   watch->edge_triggered = edge_triggered;
   watch->tag = tag;

   lwp_eventqueue_add (ctx->queue,
                       fd,
                       on_read_ready != NULL,
                       on_write_ready != NULL,
                       edge_triggered,
                       watch);

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
         || (edge_triggered != watch->edge_triggered)
         || tag != watch->tag)
   {
      lwp_eventqueue_update (ctx->queue,
                             watch->fd,
                             watch->on_read_ready != NULL, on_read_ready != NULL,
                             watch->on_write_ready != NULL, on_write_ready != NULL,
                             watch->edge_triggered, edge_triggered,
                             watch->tag, tag);
   }

   watch->on_read_ready = on_read_ready;
   watch->on_write_ready = on_write_ready;
   watch->edge_triggered = edge_triggered;
   watch->tag = tag;
}

static void def_remove (lw_pump pump, lw_pump_watch watch)
{
   lw_eventpump ctx = (lw_eventpump) pump;

   /* TODO : Should this remove the FD from the eventqueue immediately? */

   watch->on_read_ready = NULL;
   watch->on_write_ready = NULL;

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

