
/* vim: set et ts=3 sw=3 ft=c:
 *
 * Copyright (C) 2012 James McLaughlin et al.  All rights reserved.
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

void lwp_disable_ipv6_only (lwp_socket socket)
{
   int no = 0;
   setsockopt (socket, IPPROTO_IPV6, IPV6_V6ONLY, (char *) &no, sizeof (no));
}

struct sockaddr_storage lwp_socket_addr (lwp_socket socket)
{
   struct sockaddr_storage addr;
   socklen_t addr_len;

   memset (&addr, 0, sizeof (addr));

   if (socket != -1)
   {
      addr_len = sizeof (addr);
      getsockname (socket, (struct sockaddr *) &addr, &addr_len);
   }

   return addr;
}

long lwp_socket_port (lwp_socket socket)
{
   struct sockaddr_storage addr = lwp_socket_addr (socket);

   return ntohs (addr.ss_family == AF_INET6 ?
                  ((struct sockaddr_in6 *) &addr)->sin6_port
                    : ((struct sockaddr_in *) &addr)->sin_port);
}

lw_bool lwp_urldecode (const char * in, size_t in_length,
                       char * out, size_t out_length, lw_bool plus_spaces)
{
   char n [3];
   size_t cur_in = 0, cur_out = 0;

   n [2] = 0;

   while (cur_in < in_length)
   {
      if (cur_out >= out_length)
         return lw_false;

      if( (n[0] = in [cur_in]) == '%')
      {
         if ((cur_in + 2) > in_length)
            return lw_false;

         n [0] = in [++ cur_in]; 
         n [1] = in [++ cur_in];

         out [cur_out ++] = (char) strtol (n, 0, 16);
      }
      else
      {
         out [cur_out ++] = (plus_spaces && n [0] == '+') ? ' ' : n [0];
      }

      ++ cur_in;
   }

   out [cur_out] = 0;

   return lw_true;
}

lw_bool lwp_begins_with (const char * string, const char * substring)
{
   while (*substring)
   {
      if (*string ++ != *substring ++)
         return lw_false;
   }

   return lw_true;
}

void lwp_copy_string (char * dest, const char * source, size_t size)
{
   size_t length = strlen (source);

   if (length > -- size)
      length = size;

   memcpy (dest, source, length);
   dest [length] = 0;
}

lw_bool lwp_find_char (const char ** str, size_t * len, char c)
{
   while (*len > 0)
   {
      if (**str == c)
         return lw_true;

      ++ (*str);
      -- (*len);
   }

   return lw_false;
}

void lwp_close_socket (lwp_socket socket)
{
   if (socket == -1)
      return;

   #ifdef _WIN32
      CancelIo ((HANDLE) socket);
      closesocket (socket);
   #else
      close (socket);
   #endif
}

#ifdef __MINGW32__
   _CRTIMP int _vscprintf (const char * format, va_list argptr);
#endif

ssize_t lwp_format (char ** output, const char * format, va_list args)
{
   ssize_t count;

   #ifdef _WIN32

      count = _vscprintf (format, args);

      if (! (*output = (char *) malloc (count + 1)))
         return 0;

      if (vsprintf (*output, format, args) < 0)
      {
         free (*output);
         *output = 0;

         return 0;
      }

   #else

      /* TODO : Alternative for where vasprintf is not supported? */

      if ((count = vasprintf (output, format, args)) == -1)
         *output = 0;

   #endif

   return count;
}

time_t lwp_parse_time (const char * string)
{
    char copy [32];
    size_t length;
    char * month, * time;
    int day, year;
    int i = 0;
    struct tm tm;

    if ((length = strlen (string)) > (sizeof (copy) - 1))
       return 0;

    if (length < 8)
        return 0;

    memcpy (copy, string, length);
    copy [length] = 0;

    if (copy [3] == ',')
    {
        /* RFC 822/RFC 1123 - Sun, 06 Nov 1994 08:49:37 GMT */

        if (length < 29)
            return 0;

        copy [ 7] = 0;
        copy [11] = 0;
        copy [16] = 0;
        
        day = atoi (copy + 4);
        month = copy + 8;
        year = atoi (copy + 12);
        time = copy + 17;
    }
    else if (string[3] == ' ' || string[3] == '\t')
    {
        /* ANSI C asctime() format - Sun Nov  6 08:49:37 1994 */

        if (length < 24)
            return 0;

        copy [ 7] = 0;
        copy [10] = 0;
        copy [19] = 0;

        month = copy + 4;
        day = atoi (copy + 8);
        time = copy + 11;
        year = atoi (copy + 20);
    }
    else
    {
        /* RFC 850 date (Sunday, 06-Nov-94 08:49:37 GMT) - unsupported */
        
        return 0;
    }

    tm.tm_mday  = day;
    tm.tm_wday  = 0;
    tm.tm_year  = year - 1900;
    tm.tm_isdst = 0;
    tm.tm_yday  = 0;

    while (i < 12)
    {
       if (strcasecmp (lwp_months [i], month))
       {
          ++ i;
          continue;
       }

       tm.tm_mon = i;

       if(strlen (time) < 8)
           return 0;

       time [2] = 0;
       time [5] = 0;

       tm.tm_hour = atoi (time);
       tm.tm_min  = atoi (time + 3);
       tm.tm_sec  = atoi (time + 6);

       #if defined(ANDROID)
          return timegm64 (&tm);
       #elif defined (_WIN32)
          #ifndef __MINGW_H
             return _mkgmtime64 (&tm);
          #else
             return compat_mkgmtime64 () (&tm);
          #endif
       #else
          #ifdef HAVE_TIMEGM
             return timegm (&tm);
          #else
             #pragma error "Can't find a suitable way to convert a tm to a UTC UNIX time"
          #endif
       #endif
    }

    return 0;
}

void lwp_to_lowercase (char * str)
{
   char * i;

   for (i = str; *i; ++ i)
      *i = tolower (*i);
}

