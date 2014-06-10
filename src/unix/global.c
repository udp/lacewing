
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

#include "../common.h"

static lw_bool init_called = lw_false;

void lwp_init ()
{
   if (init_called)
      return;

   init_called = lw_true;

   #ifdef ENABLE_SSL

      STACK_OF (SSL_COMP) * comp_methods;

      SSL_library_init ();
 
      SSL_load_error_strings ();
      ERR_load_crypto_strings ();

      comp_methods = SSL_COMP_get_compression_methods ();
      sk_SSL_COMP_zero (comp_methods);

   #endif
}

lw_bool lw_file_exists (const char * filename)
{
   struct stat attr;

   if (stat (filename, &attr) == 0)
      return !S_ISDIR (attr.st_mode);

   return lw_false;
}

lw_bool lw_path_exists (const char * filename)
{
   struct stat attr;

   if (stat (filename, &attr) == 0)
      return S_ISDIR (attr.st_mode);

   return lw_false;
}

size_t lw_file_size (const char * filename)
{
   struct stat attr;

   if (stat (filename, &attr) == 0)
      return attr.st_size;

   return 0;
}

lw_i64 lw_last_modified (const char * filename)
{
   struct stat attr;

   if (stat (filename, &attr) == 0)
      return attr.st_mtime;

   return 0;
}

void lw_temp_path (char * buffer)
{
   char * path = getenv ("TMPDIR");

   if (path)
      strcpy (buffer, path);
   else if (P_tmpdir)
      strcpy (buffer, P_tmpdir);
   else
      strcpy (buffer, "/tmp/");

   if (*buffer && buffer [strlen (buffer) - 1] != '/')
      strcat (buffer, "/");

   return;
}

lw_bool lw_random (char * buffer, size_t size)
{
   static int dev_random = -1;

   if (dev_random == -1)
      dev_random = open ("/dev/urandom", O_RDONLY);

   if (dev_random == -1)
   {
      lwp_trace ("Error opening random: %s", strerror (errno));
      return lw_false;
   }

   if (read (dev_random, buffer, size) != size)
   {
      lwp_trace ("Error reading from random: %s", strerror (errno));
      return lw_false;
   }

   return lw_true;
}

#ifdef ENABLE_SSL

 void lw_md5 (char * output, const char * input, size_t length)
 {
    MD5_CTX context;

    MD5_Init (&context);
    MD5_Update (&context, input, length);
    MD5_Final ((unsigned char *) output, &context);
 }

 void lw_sha1 (char * output, const char * input, size_t length)
 {
    SHA_CTX context;
  
    SHA1_Init (&context);
    SHA1_Update (&context, input, length);
    SHA1_Final ((unsigned char *) output, &context);
 }

#endif

