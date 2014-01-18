
/* vim: set et ts=3 sw=3 sts=3 ft=c:
 *
 * Copyright (C) 2011, 2012, 2013 James McLaughlin.  All rights reserved.
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
#include "../address.h"
#include "ssl/clientssl.h"
#include "fdstream.h"

struct _lw_client
{
   struct _lw_fdstream fdstream;

   lw_pump pump;
   lw_pump_watch watch;

   lw_client_hook_connect     on_connect;
   lw_client_hook_disconnect  on_disconnect;
   lw_client_hook_data        on_data;
   lw_client_hook_error       on_error;

   lw_addr address;

   HANDLE socket;
   lw_bool connecting;
};

lw_client lw_client_new (lw_pump pump)
{
   lwp_init ();

   lw_client ctx = (lw_client) calloc (sizeof (*ctx), 1);

   if (!ctx)
      return 0;

   lwp_fdstream_init (&ctx->fdstream, pump);

   ctx->socket = INVALID_HANDLE_VALUE;
   ctx->pump = pump;
   
   return ctx;
}

void lw_client_delete (lw_client ctx)
{
   if (!ctx)
      return;

   lw_stream_close ((lw_stream) ctx, lw_true);

   lw_addr_delete (ctx->address);

   free (ctx);
}

void lw_client_connect (lw_client ctx, const char * host, long port)
{
   lw_addr address = lw_addr_new_port (host, port);

   lw_client_connect_addr (ctx, address);
}

static void completion (void * tag, OVERLAPPED * overlapped,
                            unsigned long bytes_transfered, int error)
{
   lw_client ctx = (lw_client) tag;

   assert (ctx->connecting);

   if (error)
   {
      ctx->connecting = lw_false;

      lw_error error = lw_error_new ();
      lw_error_addf (error, "Error connecting");

      if (ctx->on_error)
         ctx->on_error (ctx, error);

      lw_error_delete (error);

      return;
   }

   lw_fdstream_set_fd (&ctx->fdstream, ctx->socket, ctx->watch, lw_true);

   ctx->connecting = lw_false;

   if (ctx->on_connect)
      ctx->on_connect (ctx);

   if (ctx->on_data)
      lw_stream_read ((lw_stream) ctx, -1);
}

void lw_client_connect_addr (lw_client ctx, lw_addr address)
{
   if (lw_client_connected (ctx) || lw_client_connecting (ctx))
   {
      lw_error error = lw_error_new ();
      lw_error_addf (error, "Already connected to a server");

      if (ctx->on_error)
         ctx->on_error (ctx, error);

      lw_error_delete (error);

      return;
   }

   ctx->connecting = lw_true;

   /* TODO : Resolve asynchronously? */

   {  lw_error error = lw_addr_resolve (address);

      if (error)
      {
         if (ctx->on_error)
            ctx->on_error (ctx, error);

         lw_error_delete (error);

         return;
      }
   }

   lw_addr_delete (ctx->address);
   ctx->address = lw_addr_clone (address);

   if (!address->info)
   {
      lw_error error = lw_error_new ();
      lw_error_addf (error, "The provided Address object is not ready for use");

      if (ctx->on_error)
         ctx->on_error (ctx, error);

      lw_error_delete (error);

      return;
   }

   if ((ctx->socket = (HANDLE) WSASocket
            (lw_addr_ipv6 (ctx->address) ? AF_INET6 : AF_INET, SOCK_STREAM, IPPROTO_TCP,
             0, 0, WSA_FLAG_OVERLAPPED)) == INVALID_HANDLE_VALUE)
   {
      lw_error error = lw_error_new ();

      lw_error_add (error, WSAGetLastError ());
      lw_error_addf (error, "Error creating socket");

      if (ctx->on_error)
         ctx->on_error (ctx, error);

      lw_error_delete (error);

      return;
   }

   lwp_disable_ipv6_only ((lwp_socket) ctx->socket);

   ctx->watch = lw_pump_add (ctx->pump, ctx->socket, ctx, completion);

   /* LPFN_CONNECTEX and WSAID_CONNECTEX aren't defined w/ MinGW */

   static BOOL (PASCAL FAR * lw_ConnectEx)
      (   
       SOCKET s,
       const struct sockaddr FAR *name,
       int namelen,
       PVOID lpSendBuffer,
       DWORD dwSendDataLength,
       LPDWORD lpdwBytesSent,
       LPOVERLAPPED lpOverlapped
      );

   GUID ID = {0x25a207b9,0xddf3,0x4660,
      {0x8e,0xe9,0x76,0xe5,0x8c,0x74,0x06,0x3e}};

   DWORD bytes = 0;

   WSAIoctl ((SOCKET) ctx->socket, SIO_GET_EXTENSION_FUNCTION_POINTER,
         &ID, sizeof (ID), &lw_ConnectEx, sizeof (lw_ConnectEx),
         &bytes, 0, 0);

   assert (lw_ConnectEx);

   struct sockaddr_storage local_address = {};

   if (lw_addr_ipv6 (address))
   {
      ((struct sockaddr_in6 *) &local_address)->sin6_family = AF_INET6;
      ((struct sockaddr_in6 *) &local_address)->sin6_addr = in6addr_any;       
   }
   else
   {
      ((struct sockaddr_in *) &local_address)->sin_family = AF_INET;
      ((struct sockaddr_in *) &local_address)->sin_addr.S_un.S_addr = INADDR_ANY;
   }

   if (bind ((SOCKET) ctx->socket,
            (struct sockaddr *) &local_address, sizeof (local_address)) == -1)
   {
      lw_error error = lw_error_new ();

      lw_error_add (error, WSAGetLastError ());
      lw_error_addf (error, "Error binding socket");

      if (ctx->on_error)
         ctx->on_error (ctx, error);

      lw_error_delete (error);

      return;
   }

   lw_addr_delete (ctx->address);
   ctx->address = lw_addr_clone (address);

   OVERLAPPED * overlapped = (OVERLAPPED *) calloc (sizeof (*overlapped), 1);

   if (!lw_ConnectEx ((SOCKET) ctx->socket, address->info->ai_addr,
            (int) address->info->ai_addrlen, 0, 0, 0, overlapped))
   {
      int code = WSAGetLastError ();

      assert (code == WSA_IO_PENDING);
   }
}

lw_bool lw_client_connected (lw_client ctx)
{
   return lw_fdstream_valid ((lw_fdstream) ctx);
}

lw_bool lw_client_connecting (lw_client ctx)
{
   return ctx->connecting;
}

lw_addr lw_client_server_addr (lw_client ctx)
{
   return ctx->address;
}

static void on_stream_data (lw_stream stream, void * tag,
                            const char * buffer, size_t length)
{
   lw_client ctx = (lw_client) tag;

   ctx->on_data (ctx, buffer, length);
}

void lw_client_on_data (lw_client ctx, lw_client_hook_data on_data)
{
    ctx->on_data = on_data;

    if (on_data)
    {
        lw_stream_add_hook_data ((lw_stream) ctx, on_stream_data, ctx);
        lw_stream_read ((lw_stream) ctx, -1);
    }
    else
    {
        lw_stream_remove_hook_data ((lw_stream) ctx, on_stream_data, ctx);
    }
}

static void on_close (lw_stream stream, void * tag)
{
   lw_client ctx = (lw_client) tag;

   ctx->on_disconnect (ctx);
}

void lw_client_on_disconnect (lw_client ctx,
                              lw_client_hook_disconnect on_disconnect)
{
   ctx->on_disconnect = on_disconnect;

   if (on_disconnect)
      lw_stream_add_hook_close ((lw_stream) ctx, on_close, ctx);
   else
      lw_stream_remove_hook_close ((lw_stream) ctx, on_close, ctx);
}

lwp_def_hook (client, connect)
lwp_def_hook (client, error)

