
/* vim: set et ts=3 sw=3 ft=c:
 *
 * Copyright (C) 2012, 2013 James McLaughlin et al.  All rights reserved.
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

static char version [64] = { 0 };

const char * lw_version ()
{
   const char * platform;

   #ifndef _WIN32
     struct utsname name;
   #endif

   if (!*version)
   {
      #if defined (_WIN32)
         platform = "Windows";
      #elif defined (_lacewing_android)
         platform = "Android";
      #else
         uname (&name);
         platform = name.sysname;
      #endif

      #ifndef PACKAGE_VERSION
         #define PACKAGE_VERSION "0.5.4"
      #endif

      sprintf (version, "liblacewing " PACKAGE_VERSION " (%s, %d-bit)",
                     platform, ((int) sizeof(void *)) * 8);
   }

   return version;
}

const char * const lwp_weekdays [] =
    { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

const char * const lwp_months [] =
    { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

void lw_dump (const char * buffer, size_t size)
{
   const int bytes_per_row = 8;
   const int text_offset = bytes_per_row * 3 + 8;

   unsigned char b;
   lw_i64 i;
   int row_offset = 0, row_offset_c = 0, row = 0;

   if (size == -1)
      size = strlen (buffer);

   fprintf (stderr, "=== " lwp_fmt_size " bytes @ %p ===\n", size, buffer);

   while (size > 0)
   {
      i = row * bytes_per_row + row_offset;

      if (i >= size)
      {
         if (row_offset_c >= text_offset) /* printing text? */
            break;
         else
            row_offset = bytes_per_row; /* skip to printing text */
      }
      else
      {
         b = (unsigned char) buffer [i];

         row_offset_c += row_offset_c >= text_offset ?
            ( isprint (b) ? fprintf (stderr, "%c", b) : fprintf (stderr, ".") )
            : fprintf (stderr, "%02hX ", (short) b);
      }

      if ((++ row_offset) >= bytes_per_row)
      {
         row_offset = 0;

         if (row_offset_c < text_offset)
         {
            while (row_offset_c < text_offset)
               row_offset_c += fprintf (stderr, " ");
         }
         else
         {
            row_offset = row_offset_c = 0;
            ++ row;

            fprintf (stderr, "\n");
         }
      }
   }

   fprintf (stderr, "\n===\n");
}

#ifndef _lacewing_no_ssl

 void lw_md5_hex (char * output, const char * input, size_t length)
 {
     char hex [48];
     int i = 0;

     lw_md5 (output, input, length);
     
     while (i < 16)
     {
         sprintf (hex + (i * 2), "%02x", ((unsigned char *) output) [i]);
         ++ i;
     }
 
     strcpy (output, hex);
 }

 void lw_sha1_hex (char * output, const char * input, size_t length)
 {
     char hex [48];
     int i = 0;
 
     lw_sha1 (output, input, length);
     
     while (i < 20)
     {
         sprintf (hex + (i * 2), "%02x", ((unsigned char *) output) [i]);
         ++ i;
     }
 
     strcpy (output, hex);
 }

#endif

void lw_trace (const char * format, ...)
{
   va_list args;
   char * data;
   size_t size;
   static lw_sync sync = 0;

   va_start (args, format);
   
   size = lwp_format (&data, format, args);
   
   if(size > 0)
   {
      if (!sync)
         sync = lw_sync_new ();

      lw_sync_lock (sync);

      #ifdef _lacewing_android
         __android_log_write (ANDROID_LOG_INFO, "liblacewing", data);
      #else
         #ifdef COXSDK
            OutputDebugStringA (data);
            OutputDebugStringA ("\n");
         #else
            fprintf (stderr, "[liblacewing] %s\n", data);
         #endif
      #endif

      free (data);

      lw_sync_release (sync);
   }

   va_end (args);
}

