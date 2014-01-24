
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
#include "sslclient.h"
#include "../stream.h"

#define lwp_sslclient_flag_handshook   1
#define lwp_sslclient_flag_pumping     2
#define lwp_sslclient_flag_dead        4

struct _lwp_sslclient
{
   struct ssl_st * ssl;
   SSL_CTX * server_context;

   BIO * bio_internal, * bio_external;

   int write_condition;

   char flags;

   void * tag;
   lwp_sslclient_on_handshook on_handshook;

   #ifdef _lacewing_npn
      unsigned char npn [32];
   #endif

   struct _lw_stream upstream;
   struct _lw_stream downstream;
};

const static lw_streamdef def_upstream;
const static lw_streamdef def_downstream;

static void pump (lwp_sslclient);

lwp_sslclient lwp_sslclient_new (SSL_CTX * server_context, lw_stream socket,
                                 lwp_sslclient_on_handshook on_handshook,
                                 void * tag)
{
   lwp_sslclient ctx = calloc (sizeof (*ctx), 1);

   if (!ctx)
      return 0;

   ctx->write_condition = -1;

   #ifdef _lacewing_npn
      *ctx->npn = 0;
   #endif

   ctx->server_context = server_context;
   ctx->ssl = SSL_new (server_context);

   ctx->on_handshook = on_handshook;
   ctx->tag = tag;

   /* TODO : I'm not really happy with the extra layer of buffering
    * that BIO pairs introduce.  Is there a better way to do this?
    */

   BIO_new_bio_pair (&ctx->bio_internal, 0, &ctx->bio_external, 0);
   SSL_set_bio (ctx->ssl, ctx->bio_internal, ctx->bio_internal);

   SSL_set_accept_state (ctx->ssl);

   lwp_stream_init (&ctx->upstream, &def_upstream, 0);
   lwp_stream_init (&ctx->downstream, &def_downstream, 0);

   /* Retain our streams indefinitely, since we'll be in charge of releasing
    * their memory.  This doesn't stop stream_delete from working.
    */
   lwp_retain (&ctx->upstream, "lwp_sslclient upstream");
   lwp_retain (&ctx->downstream, "lwp_sslclient downstream");

   lw_stream_add_filter_upstream
      (socket, &ctx->upstream, lw_false, lw_false);

   lw_stream_add_filter_downstream
      (socket, &ctx->downstream, lw_false, lw_false);

   pump (ctx);

   assert (! (ctx->flags & lwp_sslclient_flag_dead));

   return ctx;
}

void lwp_sslclient_delete (lwp_sslclient ctx)
{
   if (!ctx)
      return;

   if (ctx->flags & lwp_sslclient_flag_pumping)
   {
      ctx->flags |= lwp_sslclient_flag_dead;
      return;
   }

   SSL_free (ctx->ssl);
   BIO_free (ctx->bio_external);
 
   lw_stream_delete (&ctx->upstream);
   lw_stream_delete (&ctx->downstream);

   free (ctx);
}

lw_bool lwp_sslclient_handshook (lwp_sslclient ctx)
{
   return ctx->flags & lwp_sslclient_flag_handshook;
}

const char * lwp_sslclient_npn (lwp_sslclient ctx)
{
   #ifdef _lacewing_npn
      return ctx->npn;
   #else
      return "";
   #endif
}

static size_t downstream_sink_data (lw_stream downstream,
                                    const char * buffer, size_t size)
{
   lwp_sslclient ctx = container_of
      (downstream, struct _lwp_sslclient, downstream);

   int bytes = BIO_write (ctx->bio_external, buffer, size);

   pump (ctx);

   if (ctx->flags & lwp_sslclient_flag_dead)
   {
      lwp_sslclient_delete (ctx);
      return size;
   }

   if (bytes < 0)
   {
      lwp_trace ("SSL downstream write error!");

      lw_stream_close (&ctx->downstream, lw_true);

      return size;
   }

   assert (bytes == size);

   /* If OpenSSL was waiting for some more incoming data before we could
    * write something, signal Stream::WriteReady to have Put called again.
    */

   if (ctx->write_condition == SSL_ERROR_WANT_READ)
   {
      ctx->write_condition = -1;

      lw_stream_retry (&ctx->upstream, lw_stream_retry_now);
   }

   return bytes;
}

static size_t upstream_sink_data (lw_stream upstream,
                                  const char * buffer, size_t size)
{
   lwp_sslclient ctx = container_of
      (upstream, struct _lwp_sslclient, upstream);

   int bytes = SSL_write (ctx->ssl, buffer, size);
   int error = bytes < 0 ? SSL_get_error (ctx->ssl, bytes) : -1;

   pump (ctx);

   if (ctx->flags & lwp_sslclient_flag_dead)
   {
      lwp_sslclient_delete (ctx);
      return size;
   }

   if (error != -1)
   {
      if (error == SSL_ERROR_WANT_READ)
         ctx->write_condition = error;

      lwp_trace ("SSL upstream write error!");

      return 0;
   }

   assert (bytes == size);

   return bytes;
}

void pump (lwp_sslclient ctx)
{
   if (ctx->flags & lwp_sslclient_flag_pumping)
      return; /* prevent stack overflow (we loop anyway) */

   ctx->flags |= lwp_sslclient_flag_pumping;

   {  char buffer [lwp_default_buffer_size];
      int bytes;

      for (;;)
      {
         if (! (ctx->flags & lwp_sslclient_flag_handshook))
         {
            if (SSL_do_handshake (ctx->ssl) > 0)
            {
               ctx->flags |= lwp_sslclient_flag_handshook;

               #ifdef _lacewing_npn

                  const unsigned char * npn = 0;
                  unsigned int npn_length = 0;

                  SSL_get0_next_proto_negotiated (ctx->ssl, &npn, &npn_length);

                  if (npn)
                  {
                     if (npn_length >= sizeof (ctx->npn))
                        npn_length = sizeof (ctx->npn) - 1;

                     memcpy (ctx->npn, npn, npn_length);
                     ctx->npn [npn_length] = 0;
                  }

               #endif

               if (ctx->on_handshook)
                  ctx->on_handshook (ctx, ctx->tag);
            }
         }

         lw_bool exit = lw_true;

         /* First check for any new data destined for the network */

         bytes = BIO_read (ctx->bio_external, buffer, sizeof (buffer));

         lwp_trace ("Pump: BIO_read returned %d", bytes);

         if (bytes > 0)
         {
            exit = lw_false;

            lw_stream_data (&ctx->upstream, buffer, bytes);

            /* Pushing data may end up destroying the SSLClient user, which
             * will then set the _dead flag.
             */

            if (ctx->flags & lwp_sslclient_flag_dead)
               return;
         }
         else
         {
            if (!BIO_should_retry (ctx->bio_external))
            {
               lwp_trace ("BIO_should_retry = false");
            }
         }


         /* Now check for any data that's been decrypted and is ready to use */

         bytes = SSL_read (ctx->ssl, buffer, sizeof (buffer));

         lwp_trace ("Pump: SSL_read returned %d", bytes);

         if (bytes > 0)
         {
            exit = lw_false;
            lw_stream_data (&ctx->downstream, buffer, bytes);

            if (ctx->flags & lwp_sslclient_flag_dead)
               return;
         }
         else
         {
            if (!bytes)
            {
               lwp_trace ("SSL shutdown!");

               lw_stream_close (&ctx->downstream, lw_true);
            }
            else
            {
               int error = SSL_get_error (ctx->ssl, bytes);

               if (error == SSL_ERROR_WANT_WRITE)
                  continue;

               if (error != SSL_ERROR_WANT_READ)
               {
                  lwp_trace ("SSL error: %s", ERR_error_string (error, 0));
               }
            }
         }

         if (BIO_ctrl_pending (ctx->bio_external) > 0)
            continue;

         if (exit)
            break;
      }
   }

   ctx->flags &= ~ lwp_sslclient_flag_pumping;
}

const static lw_streamdef def_upstream =
{
   .sink_data = upstream_sink_data
};

const static lw_streamdef def_downstream =
{
   .sink_data = downstream_sink_data
};


