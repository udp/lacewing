
/* vim: set et ts=3 sw=3 ft=c:
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

#include "../common.h"
#include "../pump.h"

extern const lw_pumpdef def_eventpump;

#define sig_exit_event_loop      ((OVERLAPPED *) 1)
#define sig_remove               ((OVERLAPPED *) 2)
#define sig_end_watcher_thread   ((OVERLAPPED *) 3)

struct _lw_pump_watch
{
   lw_pump_callback on_completion;

   void * tag;
};

struct _lw_eventpump
{  
   struct _lw_pump pump;

   HANDLE completion_port;

   void (lw_callback * on_tick_needed) (lw_eventpump);

   struct
   {
      lw_thread thread;
      OVERLAPPED * overlapped;
      DWORD bytes_transferred;
      lw_pump_watch event;
      lw_event resume_event;
      int error;

   } watcher;
};

static lw_bool process (lw_eventpump ctx, OVERLAPPED * overlapped,
                        unsigned int bytes_transferred, lw_pump_watch watch,
                        int error)
{
   if (bytes_transferred == 0xFFFFFFFF)
   {
      /* See eventpump_post */

      ((void * (*) (void *)) watch) (overlapped);

      return lw_true;
   }

   if (overlapped == sig_remove)
   {
      free (watch);
      lw_pump_remove_user ((lw_pump) ctx);

      return lw_true;
   }

   if (overlapped == sig_exit_event_loop)
      return lw_false;

   if (watch->on_completion)
      watch->on_completion (watch->tag, overlapped, bytes_transferred, error);

   return lw_true;
}

static void watcher (lw_eventpump ctx)
{
   for (;;)
   {
      ctx->watcher.error = 0;

      if (!GetQueuedCompletionStatus (ctx->completion_port,
                                      &ctx->watcher.bytes_transferred,
                                      (PULONG_PTR) &ctx->watcher.event,
                                      &ctx->watcher.overlapped,
                                      INFINITE))
      {
         if ((ctx->watcher.error = GetLastError ()) == WAIT_TIMEOUT)
            break;

         if (!ctx->watcher.overlapped)
            break;

         ctx->watcher.bytes_transferred = 0;
      }

      if (ctx->watcher.overlapped == sig_end_watcher_thread)
         break;

      ctx->on_tick_needed (ctx);

      lw_event_wait (ctx->watcher.resume_event, -1);
      lw_event_unsignal (ctx->watcher.resume_event);
   }
}

lw_eventpump lw_eventpump_new ()
{
   lwp_init ();

   lw_eventpump ctx = (lw_eventpump) calloc (sizeof (*ctx), 1);

   if (!ctx)
      return 0;

   ctx->watcher.thread = lw_thread_new ("watcher", (void *) watcher);
   ctx->watcher.resume_event = lw_event_new ();

   ctx->completion_port = CreateIoCompletionPort (INVALID_HANDLE_VALUE, 0, 4, 0);

   lwp_pump_init ((lw_pump) ctx, &def_eventpump);

   return ctx;
}

static void def_cleanup (lw_pump _ctx)
{
   lw_eventpump ctx = (lw_eventpump) _ctx;

   if (lw_thread_started (ctx->watcher.thread))
   {
      PostQueuedCompletionStatus (ctx->completion_port, 0, 0, sig_end_watcher_thread);

      lw_event_signal (ctx->watcher.resume_event);
      lw_thread_join (ctx->watcher.thread);
   }

   lw_event_delete (ctx->watcher.resume_event);
}

lw_error lw_eventpump_tick (lw_eventpump ctx)
{
   if (ctx->on_tick_needed)
   {
      /* Process whatever the watcher thread dequeued before telling the caller to tick */

      process (ctx, ctx->watcher.overlapped,
                    ctx->watcher.bytes_transferred,
                    ctx->watcher.event,
                    ctx->watcher.error); 
   }

   OVERLAPPED * overlapped;
   DWORD bytes_transferred;

   lw_pump_watch watch;

   for (;;)
   {
      int error = 0;

      if (!GetQueuedCompletionStatus (ctx->completion_port,
                                      &bytes_transferred,
                                      (PULONG_PTR) &watch,
                                      &overlapped,
                                      0))
      {
         error = GetLastError ();

         if (error == WAIT_TIMEOUT)
            break;

         if (!overlapped)
            break;
      }

      process (ctx, overlapped, bytes_transferred, watch, error);
   }

   if (ctx->on_tick_needed)
      lw_event_signal (ctx->watcher.resume_event);

   return 0;
}

lw_error lw_eventpump_start_eventloop (lw_eventpump ctx)
{
   OVERLAPPED * overlapped;
   DWORD bytes_transferred;

   lw_pump_watch watch;

   for (;;)
   {
      /* TODO : Use GetQueuedCompletionStatusEx where available */

      int error = 0;

      if (!GetQueuedCompletionStatus (ctx->completion_port,
                                      &bytes_transferred,
                                      (PULONG_PTR) &watch,
                                      &overlapped, INFINITE))
      {
         error = GetLastError();

         if (!overlapped)
            continue;
      }

      if (!process (ctx, overlapped, bytes_transferred, watch, error))
         break;
   }

   return 0;
}

void lw_eventpump_post_eventloop_exit (lw_eventpump ctx)
{
    PostQueuedCompletionStatus (ctx->completion_port, 0, 0, sig_exit_event_loop);
}

lw_error lw_eventpump_start_sleepy_ticking
    (lw_eventpump ctx, void (lw_callback * on_tick_needed) (lw_eventpump))
{
    ctx->on_tick_needed = on_tick_needed;    
    lw_thread_start (ctx->watcher.thread, ctx);

    return 0;
}

static lw_pump_watch def_add (lw_pump _ctx, HANDLE handle,
                              void * tag, lw_pump_callback callback)
{
   lw_eventpump ctx = (lw_eventpump) _ctx;

   assert (callback != 0);

   lw_pump_watch watch = (lw_pump_watch) calloc (sizeof (*watch), 1);

   if (!watch)
      return 0;

   watch->on_completion = callback;
   watch->tag = tag;

   if (CreateIoCompletionPort (handle, ctx->completion_port,
                               (ULONG_PTR) watch, 0) != 0)
   {
      /* success */
   }
   else
   {
      assert (0);
   }

   lw_pump_add_user ((lw_pump) ctx);

   return watch;
}

static void def_remove (lw_pump _ctx, lw_pump_watch watch)
{
   lw_eventpump ctx = (lw_eventpump) _ctx;

   watch->on_completion = 0;

   PostQueuedCompletionStatus (ctx->completion_port, 0,
                               (ULONG_PTR) watch, sig_remove);
}

static void def_post (lw_pump _ctx, void * function, void * parameter)
{
   lw_eventpump ctx = (lw_eventpump) _ctx;

   PostQueuedCompletionStatus (ctx->completion_port, 0xFFFFFFFF,
                               (ULONG_PTR) function,
                               (OVERLAPPED *) parameter);
}

static void def_update_callbacks (lw_pump ctx, 
                                  lw_pump_watch watch,
                                  void * tag,
                                  lw_pump_callback on_completion)
{
   watch->tag = tag;
   watch->on_completion = on_completion;
}

const lw_pumpdef def_eventpump =
{
    def_add,
    def_update_callbacks,
    def_remove,
    def_post,
    def_cleanup
};

