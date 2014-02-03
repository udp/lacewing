
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

#include "common.h"

struct _lw_error
{
   char buffer [4096];
   char * begin;

   int size;

   void * tag;
};

static void lwp_error_add (lw_error ctx, const char * buffer)
{
   size_t length = strlen (buffer);

   if ((ctx->begin - length) < ctx->buffer)
      return;

   ctx->begin -= length;
   memcpy (ctx->begin, buffer, length);
}

void lw_error_addv (lw_error ctx, const char * format, va_list args)
{
   ++ ctx->size;

   if (*ctx->begin)
      lwp_error_add (ctx, " - ");

   char * buffer = (char *) malloc (sizeof (ctx->buffer) + 1);

   vsnprintf (buffer, sizeof (ctx->buffer), format, args);
   lwp_error_add (ctx, buffer);

   free (buffer);
}

lw_error lw_error_new ()
{
   lw_error ctx = (lw_error) malloc (sizeof (*ctx));

   if (!ctx)
      return 0;

   *(ctx->begin = ctx->buffer + sizeof (ctx->buffer) - 1) = 0;

   ctx->size = 0;

   return ctx;
}

void lw_error_delete (lw_error ctx)
{
   if (!ctx)
      return;

   free (ctx);
}

const char * lw_error_tostring (lw_error ctx)
{
   return ctx->begin;
}

lw_error lw_error_clone (lw_error ctx)
{
   lw_error error = (lw_error) malloc (sizeof (*error));

   if (!error)
      return 0;

   memcpy (error, ctx, sizeof (*error));

   return error;
}

void lw_error_addf (lw_error ctx, const char * format, ...)
{
   va_list args;
   va_start (args, format);

   lw_error_addv (ctx, format, args);

   va_end (args);
}

void lw_error_add (lw_error ctx, long error)
{
   #ifdef _WIN32

      char * message;

      if (FormatMessageA
            (FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
             0,
             error,
             MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
             (char *) &message,
             1,
             0))
      {
         lw_error_addf (ctx, error < 0 ? "%s (%08X)" : "%s (%d)",
               message, error);
      }

      LocalFree (message);

   #else

      lw_error_addf (ctx, "%s", strerror (error));
        
   #endif
}

size_t lw_error_size (lw_error ctx)
{
   return ctx->size;
}

void lw_error_set_tag (lw_error ctx, void * tag)
{
   ctx->tag = tag;
}

void * lw_error_tag (lw_error ctx)
{
   return ctx->tag;
}

