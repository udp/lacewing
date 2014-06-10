
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

#include "common.h"

static void on_connect (lw_server server, lw_server_client client_socket)
{
   lw_ws ws = (lw_ws) lw_server_tag (server);
   lw_bool secure = (server == ws->socket_secure);

   lwp_ws_client client;

   do
   {
      #ifdef ENABLE_SPDY

         if (!strcasecmp (lw_server_client_npn (client_socket), "spdy/3"))
         {
            client = lwp_ws_spdyclient_new (ws, client_socket, secure, 3);
            break;
         }

         if (!strcasecmp (lw_server_client_npn (client_socket), "spdy/2"))
         {
            client = lwp_ws_spdyclient_new (ws, client_socket, secure, 2);
            break;
         }

      #endif

      client = lwp_ws_httpclient_new (ws, client_socket, secure);

   } while (0);

   if (!client)
   {
      lw_stream_close ((lw_stream) client_socket, lw_true);
      return;
   }

   lw_stream_set_tag ((lw_stream) client_socket, client);

   lw_stream_write_stream
      ((lw_stream) client, (lw_stream) client_socket, -1, lw_false);
}

static void on_disconnect (lw_server server, lw_server_client client_socket)
{
   lwp_ws_client client = (lwp_ws_client) lw_stream_tag ((lw_stream) client_socket);

   assert (client);

   client->cleanup (client);
   lw_stream_delete ((lw_stream) client);

   lw_stream_set_tag ((lw_stream) client_socket, 0);
}

static void on_error (lw_server server, lw_error error)
{
    lw_error_addf (error, "Socket error");

    lw_ws ws = (lw_ws) lw_server_tag (server);

    if (ws->on_error)
        ws->on_error (ws, error);
}

static void start_timer (lw_ws ctx)
{
   #ifdef LacewingTimeoutExperiment

      if (lw_timer_started (ctx->timer))
         return;

      lw_timer_start (ctx->timer, ctx->timeout * 1000);

   #endif
}

static void on_timer_tick (lw_timer timer)
{
   lw_server_client client_socket;
   lwp_ws_client client;
   lw_ws ws = (lw_ws) lw_timer_tag (timer);

   for (client_socket = lw_server_client_first (ws->socket);
         client_socket;
         client_socket = lw_server_client_next (client_socket))
   {
      client = (lwp_ws_client) lw_stream_tag ((lw_stream) client_socket);

      client->tick (client);
   }

   for (client_socket = lw_server_client_first (ws->socket_secure);
         client_socket;
         client_socket = lw_server_client_next (client_socket))
   {
      client = (lwp_ws_client) lw_stream_tag ((lw_stream) client_socket);

      client->tick (client);
   }
}

lw_ws lw_ws_new (lw_pump pump)
{
   lw_ws ctx = (lw_ws) calloc (sizeof (*ctx), 1);

   if (!ctx)
      return 0;

   lwp_init ();

   ctx->pump = pump;
   ctx->auto_finish = lw_true;
   ctx->timeout = 5;

   ctx->timer = lw_timer_new (ctx->pump);
   lw_timer_set_tag (ctx->timer, ctx);
   lw_timer_on_tick (ctx->timer, on_timer_tick);

   ctx->socket = lw_server_new (pump);
   lw_server_set_tag (ctx->socket, ctx);

   lw_server_on_connect (ctx->socket, on_connect);
   lw_server_on_disconnect (ctx->socket, on_disconnect);
   lw_server_on_error (ctx->socket, on_error);

   ctx->socket_secure = lw_server_new (pump);
   lw_server_set_tag (ctx->socket_secure, ctx);

   lw_server_on_connect (ctx->socket_secure, on_connect);
   lw_server_on_disconnect (ctx->socket_secure, on_disconnect);
   lw_server_on_error (ctx->socket_secure, on_error);

   #ifdef ENABLE_SPDY
      lw_server_add_npn (ctx->socket_secure, "spdy/3");
      lw_server_add_npn (ctx->socket_secure, "spdy/2");
   #endif

   lw_server_add_npn (ctx->socket_secure, "http/1.1");
   lw_server_add_npn (ctx->socket_secure, "http/1.0");

   start_timer (ctx);

   return ctx;
}

void lw_ws_delete (lw_ws ctx)
{
   if (!ctx)
      return;

   lw_ws_unhost (ctx);
   lw_ws_unhost_secure (ctx);

   lw_server_delete (ctx->socket);
   lw_server_delete (ctx->socket_secure);

   lw_timer_delete (ctx->timer);

   free (ctx);
}

void lw_ws_host (lw_ws ctx, long port)
{
   lw_filter filter = lw_filter_new ();
   lw_filter_set_local_port (filter, port);

   lw_ws_host_filter (ctx, filter);

   lw_filter_delete (filter);
}

void lw_ws_host_filter (lw_ws ctx, lw_filter filter)
{
   if (!lw_filter_local_port (filter))
      lw_filter_set_local_port (filter, 80);

   lw_server_host_filter (ctx->socket, filter);
}

void lw_ws_host_secure (lw_ws ctx, long port)
{
   lw_filter filter = lw_filter_new ();
   lw_filter_set_local_port (filter, port);

   lw_ws_host_secure_filter (ctx, filter);

   lw_filter_delete (filter);
}

void lw_ws_host_secure_filter (lw_ws ctx, lw_filter filter)
{
   if(!lw_ws_cert_loaded (ctx))
      return;

   if (!lw_filter_local_port (filter))
      lw_filter_set_local_port (filter, 443);

   lw_server_host_filter (ctx->socket_secure, filter);
}

void lw_ws_unhost (lw_ws ctx)
{
   lw_server_unhost (ctx->socket);
}

void lw_ws_unhost_secure (lw_ws ctx)
{
   lw_server_unhost (ctx->socket_secure);
}

lw_bool lw_ws_hosting (lw_ws ctx)
{
   return lw_server_hosting (ctx->socket);
}

lw_bool lw_ws_hosting_secure (lw_ws ctx)
{
   return lw_server_hosting (ctx->socket_secure);
}

long lw_ws_port (lw_ws ctx)
{
   return lw_server_port (ctx->socket);
}

long lw_ws_port_secure (lw_ws ctx)
{
   return lw_server_port (ctx->socket_secure);
}

lw_bool lw_ws_load_cert_file (lw_ws ctx, const char * filename,
                              const char * passphrase)
{
   return lw_server_load_cert_file (ctx->socket_secure, filename, passphrase);
}

lw_bool lw_ws_load_sys_cert (lw_ws ctx, const char * store_name,
                                        const char * common_name,
                                        const char * location)
{
   return lw_server_load_sys_cert (ctx->socket_secure, store_name,
                                   common_name, location);
}

lw_bool lw_ws_cert_loaded (lw_ws ctx)
{
   return lw_server_cert_loaded (ctx->socket_secure);
}

void lw_ws_enable_manual_finish (lw_ws ctx)
{
   ctx->auto_finish = lw_false;
}

void lw_ws_set_idle_timeout (lw_ws ctx, long seconds)
{
   ctx->timeout = seconds;

   if (lw_timer_started (ctx->timer))
   {
      lw_timer_stop (ctx->timer);
      start_timer (ctx);
   }
}

long lw_ws_idle_timeout (lw_ws ctx)
{
   return ctx->timeout;
}

void * lw_ws_tag (lw_ws ctx)
{
   return ctx->tag;
}

void lw_ws_set_tag (lw_ws ctx, void * tag)
{
   ctx->tag = tag;
}

lwp_def_hook (ws, get)
lwp_def_hook (ws, post)
lwp_def_hook (ws, head)
lwp_def_hook (ws, error)
lwp_def_hook (ws, upload_start)
lwp_def_hook (ws, upload_chunk)
lwp_def_hook (ws, upload_done)
lwp_def_hook (ws, upload_post)
lwp_def_hook (ws, disconnect)

