
/* vim: set et ts=3 sw=3 ft=c:
 *
 * Copyright (C) 2011, 2012, 2013 James McLaughlin et al.  All rights reserved.
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

struct _lw_event
{
   int pipe_r, pipe_w;

   void * tag;
};

lw_event lw_event_new ()
{
   lw_event ctx = malloc (sizeof (*ctx));

   int p [2];
   pipe (p);

   ctx->pipe_r = p [0];
   ctx->pipe_w = p [1];
        
   fcntl (ctx->pipe_r, F_SETFL, fcntl (ctx->pipe_r, F_GETFL, 0) | O_NONBLOCK);

   return ctx;
}

void lw_event_delete (lw_event ctx)
{
   if (!ctx)
      return;

   close (ctx->pipe_w);
   close (ctx->pipe_r);
}

lw_bool lw_event_signalled (lw_event ctx)
{
   fd_set set;

   FD_ZERO (&set);
   FD_SET (ctx->pipe_r, &set);

   struct timeval timeout = { 0 };

   return select (ctx->pipe_r + 1, &set, 0, 0, &timeout) > 0;
}

void lw_event_signal (lw_event ctx)
{
   write (ctx->pipe_w, "", 1);
}

void lw_event_unsignal (lw_event ctx)
{
   char buf [16];

   while (read (ctx->pipe_r, buf, sizeof (buf)) != -1)
   {
   }
}

lw_bool lw_event_wait (lw_event ctx, long timeout)
{      
    fd_set set;

    FD_ZERO (&set);
    FD_SET (ctx->pipe_r, &set);

    if (timeout == -1)
    {
       return select (ctx->pipe_r + 1, &set, 0, 0, 0) > 0;
    }
    else
    {
       struct timeval tv;

       tv.tv_sec = timeout / 1000;
       tv.tv_usec = (timeout % 1000) * 1000;

       return select (ctx->pipe_r + 1, &set, 0, 0, &tv) > 0;
    }
}

void lw_event_set_tag (lw_event ctx, void * tag)
{
   ctx->tag = tag;
}

void * lw_event_tag (lw_event ctx)
{
   return ctx->tag;
}

