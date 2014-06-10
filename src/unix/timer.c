
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

struct _lw_timer
{
   lw_pump pump;

   lw_timer_hook_tick on_tick;

   void * tag;

   lw_bool started;

   #ifdef _lacewing_use_timerfd
      int fd;
   #endif

   lw_event stop_event;
   long interval;

   lw_thread timer_thread;
};

static void timer_tick (lw_timer ctx)
{
  if (ctx->on_tick)
      ctx->on_tick (ctx);

  #ifdef _lacewing_use_timerfd
     lw_i64 expirations;
     read (ctx->fd, &expirations, sizeof (lw_i64));
  #endif
}

static void timer_thread (void * ptr)
{
   lw_timer ctx = ptr;

   for (;;)
   {
      lw_event_wait (ctx->stop_event, ctx->interval);

      if (lw_event_signalled (ctx->stop_event))
         break;

      lw_pump_post (ctx->pump, timer_tick, ctx);
   }
}

lw_timer lw_timer_new (lw_pump pump)
{
   lw_timer ctx = calloc (sizeof (*ctx), 1);

   if (!ctx)
      return 0;

   ctx->pump = pump;
   ctx->timer_thread = lw_thread_new ("timer_thread", timer_thread);
   ctx->stop_event = lw_event_new ();

   #ifdef _lacewing_use_timerfd
      ctx->fd = timerfd_create (CLOCK_MONOTONIC, TFD_NONBLOCK);
      lw_pump_add (ctx->pump, ctx->fd, ctx, (lw_pump_callback) timer_tick, 0, lw_true);
   #endif

   return ctx;
}

void lw_timer_delete (lw_timer ctx)
{
   lw_timer_stop (ctx);
   lw_event_delete (ctx->stop_event);

   #ifdef _lacewing_use_timerfd
      close (ctx->fd);
   #endif

   free (ctx);
}

void lw_timer_start (lw_timer ctx, long interval)
{
   lw_timer_stop (ctx);

   ctx->started = lw_true;
   lw_pump_add_user (ctx->pump);

   #ifdef USE_KQUEUE
    
      if (ctx->pump->def == &def_eventpump)
      {
         struct kevent event;

         EV_SET (&event, (uintptr_t) ctx, EVFILT_TIMER,
                    EV_ADD | EV_ENABLE | EV_CLEAR, 0, interval, ctx);

         if (kevent (((lw_eventpump) ctx->pump)->queue,
                        &event, 1, 0, 0, 0) == -1)
         {
            lwp_trace ("Timer: Failed to add timer to kqueue: %s",
                            strerror (errno));

            return;
         }
      }
      else
      {
         ctx->interval = interval;
         lw_thread_start (ctx->timer_thread, ctx);
      }

   #else
      #ifdef _lacewing_use_timerfd
        
            struct itimerspec spec;

            spec.it_interval.tv_sec  = interval / 1000;
            spec.it_interval.tv_nsec = (interval % 1000) * 1000000;

            spec.it_value.tv_sec = 0;
            spec.it_value.tv_nsec = 1;
 
            timerfd_settime (ctx->fd, 0, &spec, 0);
            
      #else
            ctx->interval = interval;
            lw_thread_start (ctx->timer_thread, ctx);
      #endif
   #endif
}

void lw_timer_stop (lw_timer ctx)
{
   if (!lw_timer_started (ctx))
      return;

   /* TODO: What if a tick has been posted and this gets destructed? */

   #ifndef _lacewing_use_timerfd

      lw_event_signal (ctx->stop_event);
      lw_thread_join (ctx->timer_thread);
      lw_event_unsignal (ctx->stop_event);

    #endif

    #ifdef USE_KQUEUE

      if (ctx->pump->def == &def_eventpump)
      {
         struct kevent event;

         EV_SET (&event, (uintptr_t) ctx, EVFILT_TIMER, EV_DELETE, 0, 0, ctx);

         if (kevent (((lw_eventpump) ctx->pump)->queue,
                        &event, 1, 0, 0, 0) == -1)
         {
            lwp_trace ("Timer: Failed to remove timer from kqueue: %s",
                            strerror (errno));

            return;
         }
      }
      else
      {
         /* TODO */
      }

    #else
        #ifdef _lacewing_use_timerfd
           struct itimerspec spec = {};
           timerfd_settime (ctx->fd, 0, &spec, 0);
        #endif
    #endif

   ctx->started = lw_false;
   lw_pump_remove_user (ctx->pump);
}

void lw_timer_force_tick (lw_timer ctx)
{
   if (ctx->on_tick)
      ctx->on_tick (ctx);
}

lw_bool lw_timer_started (lw_timer ctx)
{
   return ctx->started;
}

void lw_timer_set_tag (lw_timer ctx, void * tag)
{
   ctx->tag = tag;
}

void * lw_timer_tag (lw_timer ctx)
{
   return ctx->tag;
}

lwp_def_hook (timer, tick)

