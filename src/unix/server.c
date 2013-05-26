
/* vim: set et ts=3 sw=3 ft=c:
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

#ifndef _lacewing_no_ssl
   #include "../openssl/sslclient.h"
#endif

#include "../address.h"

#include "fdstream.h"

static void on_client_close (lw_stream, void * tag);

static void on_client_data (lw_stream, void * tag, const char * buffer,
                            size_t size);

#ifndef _lacewing_no_ssl
   static void on_ssl_handshook (lwp_sslclient ssl, void * tag);
#endif

struct _lw_server
{
   int socket; 

   lw_pump pump;
    
   lw_server_hook_connect on_connect;
   lw_server_hook_disconnect on_disconnect;
   lw_server_hook_data on_data;
   lw_server_hook_error on_error;

   void * tag;

   #ifndef _lacewing_no_ssl
      SSL_CTX * ssl_context;
      char ssl_passphrase [128];

      #ifdef _lacewing_npn
         unsigned char npn [128];
      #endif
   #endif

   list (lw_server_client, clients);
};
    
struct _lw_server_client
{
   struct _lw_fdstream fdstream;

   lw_server server;

   lw_bool on_connect_called;

   #ifndef _lacewing_no_ssl
      lwp_sslclient ssl;
   #endif

   lw_addr address;

   int fd;
   lw_pump_watch watch;

   lw_server_client * elem;
};

static lw_server_client lwp_server_client_new (lw_server ctx, lw_pump pump, int fd)
{
   lw_server_client client = calloc (sizeof (*client), 1);

   if (!client)
      return 0;

   client->server = ctx;

   lwp_fdstream_init (&client->fdstream, pump);

   /* We keep this reference right up until the client disconnects from
    * the server
    */
   lwp_retain (client);

   /* The first added close hook is always the last called.
    * This is important, because ours will destroy the client.
    */
   lw_stream_add_hook_close ((lw_stream) client, on_client_close, client);

   #ifndef _lacewing_no_ssl

      if (ctx->ssl_context)
      {
         client->ssl = lwp_sslclient_new (ctx->ssl_context, (lw_stream) client,
                                          on_ssl_handshook, client);
      }

    #endif

   lw_fdstream_set_fd (&client->fdstream, fd, 0, lw_true);

   return client;
}

#ifndef _lacewing_no_ssl

 void on_ssl_handshook (lwp_sslclient ssl, void * tag)
 {
    lw_server_client client = tag;
    lw_server server = client->server;

    #ifdef _lacewing_npn
       lwp_trace ("on_ssl_handshook for %p, NPN is %s",
             client, lwp_sslclient_npn (ssl));
    #endif
   
    client->on_connect_called = lw_true;

    lwp_retain (client);

    if (server->on_connect)
       server->on_connect (server, client);

    if (lwp_release (client) ||
            ((lw_stream) client)->flags & lwp_stream_flag_dead)
    {
       /* Client was deleted by connect hook
        */
       return;
    }

    list_push (server->clients, client);
    client->elem = list_elem_back (server->clients);
 }

#endif

lw_server lw_server_new (lw_pump pump)
{
   lwp_init ();

   lw_server ctx = calloc (sizeof (*ctx), 1);

   if (!ctx)
      return 0;

   ctx->pump = pump;

   #ifdef _lacewing_npn
      lwp_trace ("NPN is available\n");
   #else
      lwp_trace ("NPN is NOT available\n");
   #endif
    
   ctx->socket = -1;

   return ctx;
}

void lw_server_delete (lw_server ctx)
{
   if (!ctx)
      return;

   lw_server_unhost (ctx);

   free (ctx);
}

void lw_server_set_tag (lw_server ctx, void * tag)
{
   ctx->tag = tag;
}

void * lw_server_tag (lw_server ctx)
{
   return ctx->tag;
}

static void listen_socket_read_ready (void * tag)
{
   lw_server ctx = tag;

   struct sockaddr_storage address;
   socklen_t address_length = sizeof (address);
    
   for (;;)
   {
      int fd;

      lwp_trace ("Trying to accept...");

      if ((fd = accept (ctx->socket, (struct sockaddr *) &address,
                        &address_length)) == -1)
      {
         lwp_trace ("Failed to accept: %s", strerror (errno));
         break;
      }

      lwp_trace ("Accepted FD %d", fd);

      lw_server_client client = lwp_server_client_new (ctx, ctx->pump, fd);

      if (!client)
      {
         lwp_trace ("Failed allocating client");
         break;
      }

      client->address = lwp_addr_new_sockaddr ((struct sockaddr *) &address);

      lw_bool should_read = lw_false;

      if (ctx->on_data)
      {
         lw_stream_add_hook_data ((lw_stream) client, on_client_data, client);
         should_read = lw_true;
      }
      
      #ifndef _lacewing_no_ssl
      if (!client->ssl)
      {
      #endif

         client->on_connect_called = lw_true;

         lwp_retain (client);

         if (ctx->on_connect)
            ctx->on_connect (ctx, client);

         if (lwp_release (client) ||
                ((lw_stream) ctx)->flags & lwp_stream_flag_dead)
         {
            /* Client was deleted by connect hook
             */
            return;
         }

         list_push (ctx->clients, client);
         client->elem = list_elem_back (ctx->clients);

      #ifndef _lacewing_no_ssl
      }
      else
      {
         should_read = lw_true;
      }
      #endif

      if (should_read)
      {
         lwp_retain (client);

         lw_stream_read ((lw_stream) client, -1);

         if (lwp_release (client) ||
                ((lw_stream) client)->flags & lwp_stream_flag_dead)
         {
            /* Client was deleted when performing initial read
             */
            return;
         }
      }
   }
}

void lw_server_host (lw_server ctx, long port)
{
   lw_filter filter = lw_filter_new ();
   lw_filter_set_local_port (filter, port);

   lw_server_host_filter (ctx, filter);
}

void lw_server_host_filter (lw_server ctx, lw_filter filter)
{
   lw_server_unhost (ctx);

   lw_error error = lw_error_new ();

   if ((ctx->socket = lwp_create_server_socket
            (filter, SOCK_STREAM, IPPROTO_TCP, error)) == -1)
   {
      lwp_trace ("server: error hosting: %s", lw_error_tostring (error));

      if (ctx->on_error)
         ctx->on_error (ctx, error);

      return;
   }

   if (listen (ctx->socket, SOMAXCONN) == -1)
   {
      lw_error error = lw_error_new ();

      lw_error_add (error, errno);
      lw_error_addf (error, "Error listening");

      if (ctx->on_error)
         ctx->on_error (ctx, error);

      lw_error_delete (error);

      return;
   }

   lw_pump_add (ctx->pump, ctx->socket, ctx, listen_socket_read_ready, 0, lw_true);
}

void lw_server_unhost (lw_server ctx)
{
   if (!lw_server_hosting (ctx))
      return;

   close (ctx->socket);
   ctx->socket = -1;
}

lw_bool lw_server_hosting (lw_server ctx)
{
   return ctx->socket != -1;
}

size_t lw_server_num_clients (lw_server ctx)
{
   return list_length (ctx->clients);
}

long lw_server_port (lw_server ctx)
{
   return lwp_socket_port (ctx->socket);
}

lw_bool lw_server_cert_loaded (lw_server ctx)
{
   #ifndef _lacewing_no_ssl
      return ctx->ssl_context != 0;
   #else
      return lw_false;
   #endif
}

static int ssl_password_callback (char * buffer, int size, int rwflag, void * tag)
{
   #ifndef _lacewing_no_ssl

      lw_server ctx = tag;

      /* TODO : check length */

      strcpy (buffer, ctx->ssl_passphrase);
      return strlen (ctx->ssl_passphrase);

   #else
      return lw_false;
   #endif
}

#ifdef _lacewing_npn

   static int npn_advertise (SSL * ssl, const unsigned char ** data,
                             unsigned int * len, void * tag)
   {
      lw_server ctx = tag;

      *len = 0;

      for (unsigned char * i = ctx->npn; *i; )
      {
         *len += 1 + *i;
         i += 1 + *i;
      }

      *data = ctx->npn;

      lwp_trace ("Advertising for NPN...");

      return SSL_TLSEXT_ERR_OK;
   }

#endif

lw_bool lw_server_load_cert_file (lw_server ctx, const char * filename,
                                  const char * passphrase)
{
   #ifdef _lacewing_no_ssl
      return lw_false;
   #else

    SSL_load_error_strings ();

    ctx->ssl_context = SSL_CTX_new (SSLv23_server_method ());
    assert (ctx->ssl_context);

    strcpy (ctx->ssl_passphrase, passphrase);

    SSL_CTX_set_mode (ctx->ssl_context,

        SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER

        #ifdef SSL_MODE_RELEASE_BUFFERS
             | SSL_MODE_RELEASE_BUFFERS
        #endif
    );

    #ifdef _lacewing_npn
        SSL_CTX_set_next_protos_advertised_cb
            (ctx->ssl_context, npn_advertise, ctx);
    #endif

    SSL_CTX_set_quiet_shutdown (ctx->ssl_context, 1);

    SSL_CTX_set_default_passwd_cb (ctx->ssl_context, ssl_password_callback);
    SSL_CTX_set_default_passwd_cb_userdata (ctx->ssl_context, ctx);

    if (SSL_CTX_use_certificate_chain_file (ctx->ssl_context, filename) != 1)
    {
        lwp_trace ("Failed to load certificate chain file: %s",
                        ERR_error_string (ERR_get_error(), 0));

        ctx->ssl_context = 0;
        return lw_false;
    }

    if (SSL_CTX_use_PrivateKey_file (ctx->ssl_context, filename,
                                     SSL_FILETYPE_PEM) != 1)
    {
        lwp_trace ("Failed to load private key file: %s",
                        ERR_error_string (ERR_get_error(), 0));

        ctx->ssl_context = 0;
        return lw_false;
    }

    return lw_true;

   #endif
}

lw_bool lw_server_load_sys_cert (lw_server ctx,
                                 const char * store_name,
                                 const char * common_name,
                                 const char * location)
{
   lw_error error = lw_error_new ();
   lw_error_addf (error, "System certificates are only supported on Windows");

   if (ctx->on_error)
      ctx->on_error (ctx, error);

   return lw_false;
}

lw_bool lw_server_can_npn (lw_server ctx)
{
   #ifdef _lacewing_npn
      return lw_true;
   #endif

   return lw_false;
}

void lw_server_add_npn (lw_server ctx, const char * protocol)
{
   #ifdef _lacewing_npn

      size_t length = strlen (protocol);

      if (length > 0xFF)
      {
         lwp_trace ("NPN protocol too long: %s", protocol);
         return;
      }

      unsigned char * end = ctx->npn;

      while (*end)
         end += 1 + *end;

      if ((end + length + 2) > (ctx->npn + sizeof (ctx->npn)))
      {
         lwp_trace ("NPN list would have overflowed adding %s", protocol);
         return;
      }

      *end ++ = ((unsigned char) length);
      memcpy (end, protocol, length + 1);

   #endif
}

const char * lw_server_client_npn (lw_server_client client)
{
   #ifndef _lacewing_npn
      return "";
   #else

      if (client->ssl)
         return lwp_sslclient_npn (client->ssl);

      return "";

   #endif
}

lw_addr lw_server_client_addr (lw_server_client client)
{
   return client->address;
}

lw_server_client lw_server_client_next (lw_server_client client)
{
   return *list_elem_next (client->elem);
}

lw_server_client lw_server_client_first (lw_server ctx)
{
   return list_front (ctx->clients);
}

void on_client_data (lw_stream stream, void * tag, const char * buffer, size_t size)
{
   lw_server_client client = tag;
   lw_server server = client->server;

   #ifndef _lacewing_no_ssl
      assert ( (!client->ssl) || lwp_sslclient_handshook (client->ssl) );
   #endif

   assert (server->on_data);

   server->on_data (server, client, buffer, size);
}

void on_client_close (lw_stream stream, void * tag)
{
   lw_server_client client = tag;

   lw_server ctx = client->server;

   lwp_trace ("Close %d", client);

   client->fd = -1;

   if (client->on_connect_called)
   {
      if (ctx->on_disconnect)
         ctx->on_disconnect (ctx, client);
   }

   if (client->elem)
      list_elem_remove (client->elem);

   #ifndef _lacewing_no_ssl
      if (client->ssl)
         lwp_sslclient_delete (client->ssl);
   #endif

   lw_stream_delete ((lw_stream) client);

   lwp_release (client);
}

void lw_server_on_data (lw_server ctx, lw_server_hook_data on_data)
{
   ctx->on_data = on_data;

   if (on_data)
   {
      /* Setting on_data to a handler */

      if (!ctx->on_data)
      {
         list_each (ctx->clients, client)
         {
            lw_stream_add_hook_data ((lw_stream) client, on_client_data, client);
            lw_stream_read ((lw_stream) client, -1);
         }
      }

      return;
   }

   /* Setting on_data to 0 */

   list_each (ctx->clients, client)
   {
      lw_stream_remove_hook_data ((lw_stream) client, on_client_data, client);
   }
}

lwp_def_hook (server, connect)
lwp_def_hook (server, disconnect)
lwp_def_hook (server, error)

