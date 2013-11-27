
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

struct _lw_thread
{
   void * proc, * param;
   char * name;

   HANDLE thread;

   void * tag;
};

lw_thread lw_thread_new (const char * name, void * proc)
{
   lw_thread ctx = (lw_thread) calloc (sizeof (*ctx), 1);

   if (!ctx)
      return 0;

   ctx->proc = proc;
   ctx->thread = INVALID_HANDLE_VALUE;
   ctx->name = strdup (name);

   return ctx;
}

void lw_thread_delete (lw_thread ctx)
{
   if (!ctx)
      return;

   lw_thread_join (ctx);

   free (ctx);
}

static int thread_proc (lw_thread ctx)
{
   struct
   {
      DWORD dwType;
      LPCSTR szName;
      DWORD dwThreadID;
      DWORD dwFlags;

   } thread_name_info;

   thread_name_info.dwFlags     = 0;
   thread_name_info.dwType      = 0x1000;
   thread_name_info.szName      = ctx->name;
   thread_name_info.dwThreadID  = -1;

   /* TODO */

   /* __try
   {   RaiseException (0x406D1388, 0,
                       sizeof (thread_name_info) / sizeof (ULONG),
                       (ULONG *) &thread_name_info);
   }
   __except (EXCEPTION_CONTINUE_EXECUTION)
   {
   } */

   return ((int (*) (void *)) ctx->proc) (ctx->param);
}

void lw_thread_start (lw_thread ctx, void * param)
{
   if (lw_thread_started (ctx))
      return;

   ctx->param = param;

   ctx->thread = (HANDLE) _beginthreadex (0, 0,
         (unsigned (__stdcall *) (void *)) thread_proc, ctx, 0, 0);
}

lw_bool lw_thread_started (lw_thread ctx)
{
    return ctx->thread != INVALID_HANDLE_VALUE;
}

void * lw_thread_join (lw_thread ctx)
{
   if (!lw_thread_started (ctx))
      return (void *) -1;

   DWORD exit_code = -1;

   if (WaitForSingleObject (ctx->thread, INFINITE) == WAIT_OBJECT_0)
      GetExitCodeThread (ctx->thread, &exit_code);

   return (void *) exit_code;
}

void lw_thread_set_tag (lw_thread ctx, void * tag)
{
   ctx->tag = tag;
}

void * lw_thread_tag (lw_thread ctx)
{
   return ctx->tag;
}

