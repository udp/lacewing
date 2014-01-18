
/* vim: set et ts=3 sw=3 ft=c:
 *
 * Copyright (C) 2011, 2012, 2013, 2014 James McLaughlin et al.
 * All rights reserved.
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

static void timer_thread (lw_timer);

struct _lw_timer
{
   lw_pump pump;

   lw_thread timer_thread;

   HANDLE timer_handle;
   HANDLE shutdown_event;

   lw_bool started;

   lw_timer_hook_tick on_tick;

   void * tag;
};

lw_timer lw_timer_new (lw_pump pump)
{
   lw_timer ctx = (lw_timer) calloc (sizeof (*ctx), 1);

   if (!ctx)
      return 0;

   ctx->pump = pump;
   
   ctx->shutdown_event = CreateEvent (0, TRUE, FALSE, 0);
   ctx->timer_handle = CreateWaitableTimer (0, FALSE, 0);

   ctx->timer_thread = lw_thread_new ("timer", (void *) timer_thread);
   lw_thread_start (ctx->timer_thread, ctx);

   return ctx;
}

void lw_timer_delete (lw_timer ctx)
{
   if (!ctx)
      return;

   SetEvent (ctx->shutdown_event);

   lw_timer_stop (ctx);

   lw_thread_join (ctx->timer_thread);
   lw_thread_delete (ctx->timer_thread);

   free (ctx);
}

static void timer_completion (void * ptr)
{
   lw_timer ctx = (lw_timer) ptr;

   if (ctx->on_tick)
      ctx->on_tick (ctx);
}

void timer_thread (lw_timer ctx)
{
   HANDLE events [2] = { ctx->timer_handle, ctx->shutdown_event };

   for (;;)
   {
      int result = WaitForMultipleObjects (2, events, FALSE, INFINITE);

      if (result != WAIT_OBJECT_0)
      {
         lwp_trace ("Got result %d", result);
         break;
      }

      lw_pump_post (ctx->pump, (void *) timer_completion, ctx);
   }
}

void lw_timer_start (lw_timer ctx, long interval)
{
   lw_timer_stop (ctx);

   LARGE_INTEGER due_time;
   due_time.QuadPart = 0 - (interval * 1000 * 10);

   if (!SetWaitableTimer (ctx->timer_handle, &due_time, interval, 0, 0, 0))
   {
      assert (0);
   }

   ctx->started = lw_true;
   lw_pump_add_user (ctx->pump);
}

void lw_timer_stop (lw_timer ctx)
{
   if (!lw_timer_started (ctx))
      return;

   CancelWaitableTimer (ctx->timer_handle);

   ctx->started = lw_false;

   lw_pump_remove_user (ctx->pump);
}

lw_bool lw_timer_started (lw_timer ctx)
{
   return ctx->started;
}

void lw_timer_force_tick (lw_timer ctx)
{
   if (ctx->on_tick)
      ctx->on_tick (ctx);
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

