
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

#include "common.h"
#include "address.h"

static void resolver (lw_addr);

void lwp_addr_init (lw_addr ctx, const char * hostname,
                    const char * service, long hints)
{
   char * it;

   memset (ctx, 0, sizeof (*ctx));

   ctx->resolver_thread = lw_thread_new ("resolver", (void *) resolver);
   ctx->hints = hints;

   ctx->hostname_to_free = ctx->hostname = strdup (hostname);

   while (isspace (*ctx->hostname))
      ++ ctx->hostname;

   while (isspace (ctx->hostname [strlen (ctx->hostname) - 1]))
      ctx->hostname [strlen (ctx->hostname) - 1] = 0;

   for (it = ctx->hostname; *it; ++ it)
   {
      if (it [0] == ':' && it [1] == '/' && it [2] == '/')
      {
         *it = 0;

         service = ctx->hostname;
         ctx->hostname = it + 3;
      }

      if (*it == ':')
      {
         /* an explicit port overrides the service name */

         service = it + 1;
         *it = 0;
      }
   }

   lwp_copy_string (ctx->service, service, sizeof (ctx->service)); 

   lw_thread_join (ctx->resolver_thread); /* block if the thread is already running */
   lw_thread_start (ctx->resolver_thread, ctx);
}

lw_addr lw_addr_new (const char * hostname, const char * service)
{
   lw_addr ctx = (lw_addr) malloc (sizeof (*ctx));
   lwp_addr_init (ctx, hostname, service, 0);

   return ctx;
}

void lw_addr_delete (lw_addr ctx)
{
   if (!ctx)
      return;

   lw_thread_delete (ctx->resolver_thread);

   free (ctx->hostname_to_free);
   free (ctx);
}

lw_addr lw_addr_new_port (const char * hostname, long port)
{
   char service [64];

   lwp_snprintf (service, sizeof (service), "%d", port);

   lw_addr ctx = (lw_addr) malloc (sizeof (*ctx));
   lwp_addr_init (ctx, hostname, service, 0);

   return ctx;
}

lw_addr lw_addr_new_hint (const char * hostname, const char * service, long hints)
{
   lw_addr ctx = (lw_addr) malloc (sizeof (*ctx));
   lwp_addr_init (ctx, hostname, service, hints);

   return ctx;
}

lw_addr lw_addr_new_port_hint (const char * hostname, long port, long hints)
{
   char service [64];

   lwp_snprintf (service, sizeof (service), "%d", port);

   lw_addr ctx = (lw_addr) malloc (sizeof (*ctx));
   lwp_addr_init (ctx, hostname, service, hints);

   return ctx;
}

lw_addr lwp_addr_new_sockaddr (struct sockaddr * sockaddr)
{
   lw_addr addr = (lw_addr) calloc (sizeof (*addr), 1);

   if (!addr)
      return 0;
   
   lwp_addr_set_sockaddr (addr, sockaddr);

   return addr;
}

void lwp_addr_set_sockaddr (lw_addr ctx, struct sockaddr * sockaddr)
{
   if (!ctx->info)
      ctx->info = (struct addrinfo *) calloc (sizeof (*ctx->info), 1);

   ctx->info->ai_family = sockaddr->sa_family;

   free (ctx->info->ai_addr);
   ctx->info->ai_addr = (struct sockaddr *) malloc (sizeof (struct sockaddr_storage));

   switch (sockaddr->sa_family)
   {
      case AF_INET:

         ctx->info->ai_addrlen = sizeof (struct sockaddr_in);
         memcpy (ctx->info->ai_addr, sockaddr, sizeof (struct sockaddr_in));
         break;

      case AF_INET6:

         ctx->info->ai_addrlen = sizeof (struct sockaddr_in6);
         memcpy (ctx->info->ai_addr, sockaddr, sizeof (struct sockaddr_in6));
         break;
   };
}

lw_addr lw_addr_clone (lw_addr ctx)
{
   lw_addr addr = (lw_addr) calloc (sizeof (*addr), 1);

   if (!addr)
      return 0;

   addr->resolver_thread = lw_thread_new ("resolver", (void *) resolver);

   if (lw_addr_resolve (ctx))
      return 0;

   if (!ctx->info)
      return 0;

   addr->info = (struct addrinfo *) malloc (sizeof (*addr->info));
   memcpy (addr->info, ctx->info, sizeof (*addr->info));

   addr->info->ai_addrlen = ctx->info->ai_addrlen;

   addr->info->ai_next = 0;
   addr->info->ai_addr = (struct sockaddr *) malloc (addr->info->ai_addrlen);

   memcpy (addr->info->ai_addr, ctx->info->ai_addr, addr->info->ai_addrlen);

   memcpy (addr->service, ctx->service, sizeof (ctx->service));

   addr->hostname = addr->hostname_to_free = ctx->hostname;

   return addr;
}

void lwp_addr_cleanup (lw_addr ctx)
{
   if (ctx->resolver_thread)
   {
      lw_thread_join (ctx->resolver_thread);
      lw_thread_delete (ctx->resolver_thread);
   }

   free (ctx->hostname_to_free);

   lw_error_delete (ctx->error);

   if (ctx->info_list)
   {
      #ifdef _WIN32
         fn_freeaddrinfo freeaddrinfo = compat_freeaddrinfo ();
      #endif

      freeaddrinfo (ctx->info_list);
   }

   if (ctx->info_to_free)
   {
      free (ctx->info_to_free->ai_addr);
      free (ctx->info_to_free);
   }
}

const char * lw_addr_tostring (lw_addr ctx)
{
   if (!lw_addr_ready (ctx))
      return "";

   if (*ctx->buffer)
      return ctx->buffer;

   if ((!ctx->info) || (!ctx->info->ai_addr))
      return "";

   switch (ctx->info->ai_family)
   {
      case AF_INET:

         lwp_snprintf (ctx->buffer, 
                       sizeof (ctx->buffer),
                       "%s:%d",
                       inet_ntoa (((struct sockaddr_in *)
                                     ctx->info->ai_addr)->sin_addr),
                       ntohs (((struct sockaddr_in *)
                             ctx->info->ai_addr)->sin_port));

         break;

      case AF_INET6:
      {             
         int length = sizeof (ctx->buffer) - 1;

         #ifdef _WIN32

            WSAAddressToStringA ((LPSOCKADDR) ctx->info->ai_addr,
                                 (DWORD) ctx->info->ai_addrlen,
                                 0,
                                 ctx->buffer,
                                 (LPDWORD) &length);

         #else

            inet_ntop (AF_INET6,
                       &((struct sockaddr_in6 *)
                       ctx->info->ai_addr)->sin6_addr,
                       ctx->buffer,
                       length);

         #endif

            lwp_snprintf (ctx->buffer + strlen (ctx->buffer),
                          sizeof (ctx->buffer) - strlen (ctx->buffer) - 1,
                          ":%d",
                          ntohs (((struct sockaddr_in6 *)
                                ctx->info->ai_addr)->sin6_port));

            break;
       }
   };

   return ctx->buffer ? ctx->buffer: "";
}

void resolver (lw_addr ctx)
{
   struct addrinfo hints;
   int result;
   struct addrinfo * info;

   #ifdef _WIN32
      fn_getaddrinfo getaddrinfo = compat_getaddrinfo ();
   #endif

   memset (&hints, 0, sizeof (hints));

   if (ctx->hints & lw_addr_type_tcp)
   {
      assert (! (ctx->hints & lw_addr_type_udp));
      hints.ai_socktype = SOCK_STREAM;
   }
   else if (ctx->hints & lw_addr_type_udp)
   {
      hints.ai_socktype = SOCK_DGRAM;
   }

   hints.ai_protocol  =  0;
   hints.ai_flags     =  0;

   #ifdef AI_V4MAPPED
      hints.ai_flags |= AI_V4MAPPED;
   #endif
     
   #ifdef AI_ADDRCONFIG
      hints.ai_flags |= AI_ADDRCONFIG;
   #endif

   if (ctx->hints & lw_addr_hint_ipv6)
      hints.ai_family = AF_INET6;
   else
      hints.ai_family = AF_INET;

   result = getaddrinfo
      (ctx->hostname, ctx->service, &hints, &ctx->info_list);

   if (result != 0)
   {
      lw_error_delete (ctx->error);
      ctx->error = lw_error_new ();

      lw_error_addf (ctx->error, "%s", gai_strerror (result));
      lw_error_addf (ctx->error, "getaddrinfo error");

      return;
   }

   for (info = ctx->info_list; info; info = info->ai_next)
   {
      if (info->ai_family == AF_INET6)
      {
         ctx->info = info;
         break;
      }

      if (info->ai_family == AF_INET)
      {
         ctx->info = info;
         break;
      }
   }

   lw_addr_set_type (ctx, ctx->hints & (lw_addr_type_tcp | lw_addr_type_udp));
}

lw_bool lw_addr_ready (lw_addr ctx)
{
   return !ctx->resolver_thread ||
          !lw_thread_started (ctx->resolver_thread);
}

long lw_addr_port (lw_addr ctx)
{
    if ((!lw_addr_ready (ctx)) || !ctx->info || !ctx->info->ai_addr)
        return 0;

    return ntohs (ctx->info->ai_family == AF_INET6 ?
        ((struct sockaddr_in6 *) ctx->info->ai_addr)->sin6_port :
            ((struct sockaddr_in *) ctx->info->ai_addr)->sin_port);
}

void lw_addr_set_port (lw_addr ctx, long port)
{
    if ((!lw_addr_ready (ctx)) || !ctx->info || !ctx->info->ai_addr)
        return;

    *ctx->buffer = 0;

    if (ctx->info->ai_family == AF_INET6)
    {
        ((struct sockaddr_in6 *) ctx->info->ai_addr)
           ->sin6_port = htons ((unsigned short) port);
    }
    else
    {
        ((struct sockaddr_in *) ctx->info->ai_addr)
           ->sin_port = htons ((unsigned short) port);
    }
}

lw_error lw_addr_resolve (lw_addr ctx)
{
   if (ctx->resolver_thread)
      lw_thread_join (ctx->resolver_thread);

   return ctx->error;
}

static lw_bool sockaddr_equal (struct sockaddr * a, struct sockaddr * b)
{
   if ((!a) || (!b))
      return lw_false;

   if (a->sa_family == AF_INET6)
   {
      if (b->sa_family != AF_INET6)
         return lw_false;

      return !memcmp (&((struct sockaddr_in6 *) a)->sin6_addr,
                      &((struct sockaddr_in6 *) b)->sin6_addr,
                      sizeof (struct in6_addr));
   }

   if (a->sa_family == AF_INET)
   {
      if (b->sa_family != AF_INET)
         return lw_false;

      return !memcmp (&((struct sockaddr_in *) a)->sin_addr,
                      &((struct sockaddr_in *) b)->sin_addr,
                      sizeof (struct in6_addr));
   }

   return lw_false;
}

lw_bool lw_addr_equal (lw_addr a, lw_addr b)
{
   if ((!a->info) || (!b->info))
      return lw_true;

   return sockaddr_equal (a->info->ai_addr, b->info->ai_addr);
}

lw_bool lw_addr_ipv6 (lw_addr ctx)
{
   return ctx->info && ctx->info->ai_addr &&
      ((struct sockaddr_storage *) ctx->info->ai_addr)->ss_family == AF_INET6;
}

int lw_addr_type (lw_addr ctx)
{
   if (!ctx->info)
      return 0;

   switch (ctx->info->ai_socktype)
   {
      case SOCK_STREAM:
         return lw_addr_type_tcp;

      case SOCK_DGRAM:
         return lw_addr_type_udp;

      default:
         return 0;
   };
}

void lw_addr_set_type (lw_addr ctx, int type)
{
   if (!ctx->info)
      return;

   switch (type)
   {
      case lw_addr_type_tcp:
         ctx->info->ai_socktype = SOCK_STREAM;
         break;

      case lw_addr_type_udp:
         ctx->info->ai_socktype = SOCK_DGRAM;
         break;

      default:
         break;
   };
}

void * lw_addr_tag (lw_addr ctx)
{
   return ctx->tag;
}

void lw_addr_set_tag (lw_addr ctx, void * tag)
{
   ctx->tag = tag;
}

