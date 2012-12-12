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

#include "lw_common.h"

void lw_nvhash_set (lw_nvhash ** hash, const char * key, const char * value,
                    lw_bool copy)
{
   lw_nvhash_set_ex (hash, strlen (key), key, strlen (value), value, copy);
}

void lw_nvhash_set_ex (lw_nvhash ** hash, size_t key_len, const char * key,
                       size_t value_len, const char * value,
                       lw_bool copy)
{
   lw_nvhash * item;

   HASH_FIND (hh, *hash, key, key_len, item);

   if (item)
   {
      if (copy)
      {
         item->value = (char *) realloc (item->value, value_len + 1);

         memcpy (item->value, value, value_len);
         item->value [value_len] = 0;
      }
      else
      {
         free (item->value);
         item->value = (char *) value;
      }

      return;
   }

   item = (lw_nvhash *) malloc (sizeof (lw_nvhash));
   memset(item, 0, sizeof(lw_nvhash));


   if (copy)
   {
      item->key = (char *) malloc (key_len + 1);
      memcpy (item->key, key, key_len);
      item->key [key_len] = 0;

      item->value = (char *) malloc (value_len + 1);
      memcpy (item->value, value, value_len);
      item->value [value_len] = 0;
   }
   else
   {
      item->key = (char *) key;
      item->value = (char *) value;
   }

   HASH_ADD_KEYPTR (hh, *hash, item->key, key_len, item);
}

const char * lw_nvhash_get (lw_nvhash ** hash, const char * key,
                            const char * def)
{
   lw_nvhash * item;

   HASH_FIND (hh, *hash, key, strlen (key), item);

   if (item)
      return item->value;

   return def;
}

void lw_nvhash_clear (lw_nvhash ** hash)
{
   lw_nvhash * tail, * item, * tmp;

   HASH_ITER (hh, *hash, item, tmp)
   {
      HASH_DEL (*hash, item);

      free (item->key);
      free (item->value);

      free (item);
   }

   while (*hash)
   {
      tail = (lw_nvhash *) (*hash)->hh.tbl->tail;

      HASH_DEL (*hash, tail);

      free (tail->key);
      free (tail->value);
   }
}

