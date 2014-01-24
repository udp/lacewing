
/* vim: set et ts=3 sw=3 ft=c:
 *
 * Copyright (C) 2012 James McLaughlin.  All rights reserved.
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
#include "fdstream.h"

const static lw_streamdef def_fdstream;

/* FDStream makes the assumption that this will fail for anything but a regular
 * file (i.e. something that is always considered read ready)
 *
 * TODO: support more sendfile variants
 */

static lw_i64 lwp_sendfile (int source, int dest, lw_i64 size)
{
   #if defined (__linux__)

     ssize_t sent = 0;

     if ((sent = sendfile (dest, source, 0, size)) == -1)
         return errno == EAGAIN ? 0 : -1;
  
     return sent;

   #elif defined (__FreeBSD__)

     off_t sent = 0;

     if (sendfile (source, dest, lseek (source, 0, SEEK_CUR),
                     size, 0, &sent, 0) != 0)
     {
        if (errno != EAGAIN)
           return -1;
     }

     lseek (source, sent, SEEK_CUR);
     return sent;

   #elif defined (__APPLE__)

     off_t bytes = size;

     if (sendfile (source, dest, lseek (source, 0, SEEK_CUR),
                     &bytes, 0, 0) != 0)
     {
        if (errno != EAGAIN)
           return -1;
     }

     lseek (source, bytes, SEEK_CUR);
     return bytes;

   #endif

   errno = EINVAL;
   return -1;
}

static void write_ready (void * tag)
{
   lw_stream_retry ((lw_stream) tag, lw_stream_retry_now);
}

static void read_ready (void * tag)
{
   lw_fdstream ctx = tag;

   if (ctx->flags & lwp_fdstream_flag_reading)
      return;

   ctx->flags |= lwp_fdstream_flag_reading;

   lwp_retain (ctx, "fdstream read_ready");

   /* TODO : Use a buffer on the heap instead? */

   char buffer [lwp_default_buffer_size];

   lw_bool close_stream = lw_false;

   while (ctx->reading_size == -1 || ctx->reading_size > 0)
   {
      if (ctx->fd == -1)
         break;

      size_t to_read = sizeof (buffer);

      if (ctx->reading_size != -1 && to_read > ctx->reading_size)
         to_read = ctx->reading_size;

      int bytes = read (ctx->fd, buffer, to_read);

      if (bytes == 0)
      {
         close_stream = lw_true;
         break;
      }

      if (bytes == -1)
      {
         if (errno == EAGAIN)
            break;

         close_stream = lw_true;
         break;
      }

      if (ctx->reading_size != -1)
      {
         if (bytes > ctx->reading_size)
            ctx->reading_size = 0;
         else
            ctx->reading_size -= bytes;
      }

      lw_stream_data ((lw_stream) ctx, buffer, bytes);

      /* Calling Data or Close may result in destruction of the Stream -
       * see FDStream destructor.
       */

      if (! (ctx->flags & lwp_fdstream_flag_reading))
         break;
   }

   ctx->flags &= ~ lwp_fdstream_flag_reading;

   if (lwp_release (ctx, "fdstream read_ready") || ctx->stream.flags & lwp_stream_flag_dead)
      return;

   if (close_stream)
      lw_stream_close ((lw_stream) ctx, lw_true);
}

void lw_fdstream_set_fd (lw_fdstream ctx, lw_fd fd, lw_pump_watch watch,
                         lw_bool auto_close)
{
   if (ctx->watch)
   {
      lw_pump_remove (lw_stream_pump ((lw_stream) ctx), ctx->watch);
      ctx->watch = 0;
   }

   if ( (ctx->flags & lwp_fdstream_flag_autoclose) && ctx->fd != -1)
      lw_stream_close ((lw_stream) ctx, lw_true);

   ctx->fd = fd;

   if (auto_close)
      ctx->flags |= lwp_fdstream_flag_autoclose;
   else
      ctx->flags &= ~ lwp_fdstream_flag_autoclose;

   if (ctx->fd == -1)
      return;

   fcntl (fd, F_SETFL, fcntl (fd, F_GETFL, 0) | O_NONBLOCK);

   #if HAVE_DECL_SO_NOSIGPIPE
   {  int b = 1;
      setsockopt (fd, SOL_SOCKET, SO_NOSIGPIPE, (char *) &b, sizeof (b));
   }
   #endif

   {  int b = (ctx->flags & lwp_fdstream_flag_nagle) ? 0 : 1;
      setsockopt (fd, SOL_SOCKET, TCP_NODELAY, (char *) &b, sizeof (b));
   }

   struct stat stat;
   fstat (fd, &stat);

   if (S_ISSOCK (stat.st_mode))
   {
      ctx->flags |= lwp_fdstream_flag_is_socket;
   }
   else
   {
      ctx->flags &= ~ lwp_fdstream_flag_is_socket;

      if ((ctx->size = stat.st_size) > 0)
         return;

      /* Not a socket, and size is 0.  Is it really just an empty file? */

      if (S_ISREG (stat.st_mode))
         return;
   }

   /* Assuming this is something we can watch for readiness. */

   ctx->size = -1;

   lw_pump pump = lw_stream_pump ((lw_stream) ctx);

   if (watch)
   {
      /* Given an existing pump watch - change it to use our callbacks */

      ctx->watch = watch;

      lw_pump_update_callbacks (pump, ctx->watch, ctx,
            read_ready, write_ready, lw_true);
   }
   else
   {
      ctx->watch = lw_pump_add (pump, fd, ctx, read_ready,
            write_ready, lw_true);
   }
}

lw_bool lw_fdstream_valid (lw_fdstream ctx)
{
   return ctx->fd != -1;
}

void lw_fdstream_cork (lw_fdstream ctx)
{
   #ifdef lw_cork
      int enabled = 1;
      setsockopt (((lw_fdstream) ctx)->fd, IPPROTO_TCP,
                        lw_cork, &enabled, sizeof (enabled));
   #endif
}

void lw_fdstream_uncork (lw_fdstream ctx)
{
   #ifdef lw_cork
      int enabled = 0;
      setsockopt (((lw_fdstream) ctx)->fd, IPPROTO_TCP,
                        lw_cork, &enabled, sizeof (enabled));
   #endif
}

void lw_fdstream_nagle (lw_fdstream ctx, lw_bool enabled)
{
   if (enabled)
      ctx->flags |= lwp_fdstream_flag_nagle;
   else
      ctx->flags &= ~ lwp_fdstream_flag_nagle;

   if (ctx->fd != -1)
   {
      int b = enabled ? 0 : 1;
      setsockopt (ctx->fd, SOL_SOCKET, TCP_NODELAY, (char *) &b, sizeof (b));
   }
}

static size_t def_sink_data (lw_stream stream, const char * buffer, size_t size)
{
   lw_fdstream ctx = (lw_fdstream) stream;

   lwp_trace ("fdstream sink " lwp_fmt_size " bytes", size);

   size_t written;

   #if HAVE_DECL_SO_NOSIGPIPE
      written = write (ctx->fd, buffer, size);
   #else
      if (ctx->flags & lwp_fdstream_flag_is_socket)
         written = send (ctx->fd, buffer, size, MSG_NOSIGNAL);
      else
         written = write (ctx->fd, buffer, size);
   #endif

   if (written == -1)
   {
      lwp_trace ("fdstream sank nothing!  write failed: %d", errno);
      return 0;
   }

   lwp_trace ("fdstream sank " lwp_fmt_size " of " lwp_fmt_size " bytes",
              written, size);
   
   return written;
}

static size_t def_sink_stream (lw_stream _dest,
                               lw_stream _src,
                               size_t size)
{
   if (lw_stream_get_def (_src) != &def_fdstream)
      return -1;

   if (size == -1)
   {
      size = lw_stream_bytes_left (_src);

      if (size == -1)
         return -1;
   }

   lw_fdstream source = (lw_fdstream) _src;
   lw_fdstream dest = (lw_fdstream) _dest;

   lw_i64 sent = lwp_sendfile (source->fd, dest->fd, size);

   lwp_trace ("lwp_sendfile sent " lwp_fmt_size " of " lwp_fmt_size,
                     ((size_t) sent), (size_t) size);

   return sent;
}

static void def_cleanup (lw_stream _ctx)
{
   /* All of our cleanup work will already have been done by close, so there's
    * nothing to do here.
    */
}

static void def_read (lw_stream _ctx, size_t bytes)
{
   lw_fdstream ctx = (lw_fdstream) _ctx;

   lwp_trace ("FDStream %p (FD %d) read " lwp_fmt_size, ctx, ctx->fd, bytes);

   lw_bool was_reading = ctx->reading_size != 0;

   if (bytes == -1)
      ctx->reading_size = lw_stream_bytes_left ((lw_stream) ctx);
   else
      ctx->reading_size += bytes;

   if (!was_reading)
      read_ready (ctx);
}

static size_t def_bytes_left (lw_stream _ctx)
{
   lw_fdstream ctx = (lw_fdstream) _ctx;

   if (ctx->fd == -1)
      return -1; /* not valid */

   if (ctx->size == -1)
      return -1;

   lwp_trace ("lseek for FD %d, size " lwp_fmt_size, ctx->fd, ctx->size);

   /* TODO : lseek64? */

   return ctx->size - lseek (ctx->fd, 0, SEEK_CUR);
}

static lw_bool def_close (lw_stream stream_ctx, lw_bool immediate)
{
   lw_fdstream ctx = (lw_fdstream) stream_ctx;

   lwp_trace ("FDStream::Close for FD %d", ctx->fd);

   lwp_retain (ctx, "fdstream close");

   /* Remove the watch to make sure we don't get any more
    * callbacks from the pump.
    */

   if (ctx->watch)
   {
      lw_pump_remove (lw_stream_pump ((lw_stream) ctx), ctx->watch);
      ctx->watch = 0;
   }

   int fd = ctx->fd;

   ctx->fd = -1;

   if (fd != -1)
   {
      if (ctx->flags & lwp_fdstream_flag_autoclose)
      {
         shutdown (fd, SHUT_RDWR);
         close (fd);
      }
   }

   lwp_release (ctx, "fdstream close");

   return lw_true;
}

const static lw_streamdef def_fdstream =
{
   .sink_data    = def_sink_data,
   .sink_stream  = def_sink_stream,
   .close        = def_close,
   .bytes_left   = def_bytes_left,
   .read         = def_read,
   .cleanup      = def_cleanup
};

void lwp_fdstream_init (lw_fdstream ctx, lw_pump pump)
{
   memset (ctx, 0, sizeof (*ctx));

   ctx->fd = -1;
   ctx->flags = lwp_fdstream_flag_nagle;

   lwp_stream_init (&ctx->stream, &def_fdstream, pump);
}

lw_fdstream lw_fdstream_new (lw_pump pump)
{
   lw_fdstream ctx = malloc (sizeof (*ctx));
   lwp_fdstream_init (ctx, pump);

   return ctx;
}

