
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

#include "../lw_common.h"

static lw_bool init_called = lw_false;

void lwp_init ()
{
    WSADATA winsock_data;

    if (init_called)
        return;
    
    init_called = lw_true;

    WSAStartup (MAKEWORD (2, 2), &winsock_data);
}

lw_bool lw_file_exists (const char * filename)
{
   return (GetFileAttributesA (filename) & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

lw_bool lw_path_exists (const char * filename)
{
   DWORD attr = GetFileAttributesA(filename);
   return attr != 0xFFFFFFFF && (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

size_t lw_file_size (const char * filename)
{
   WIN32_FILE_ATTRIBUTE_DATA info;
   LARGE_INTEGER size;

   if (!GetFileAttributesExA (filename, GetFileExInfoStandard, &info))
      return 0;

   size.LowPart = info.nFileSizeLow;
   size.HighPart = info.nFileSizeHigh;

   return (size_t) size.QuadPart;
}

lw_i64 lw_last_modified (const char * filename)
{
   WIN32_FILE_ATTRIBUTE_DATA info;
   LARGE_INTEGER time;

   if (!GetFileAttributesExA (filename, GetFileExInfoStandard, &info))
      return 0;

   time.LowPart = info.ftLastWriteTime.dwLowDateTime;
   time.HighPart = info.ftLastWriteTime.dwHighDateTime;

   return (time_t) ((time.QuadPart - 116444736000000000) / 10000000);
}

void lw_temp_path (char * buffer)
{
   GetTempPathA (lwp_max_path, buffer);
}

static HCRYPTPROV crypt_prov = 0;

static lw_bool crypt_init ()
{
   if (!crypt_prov)
      CryptAcquireContext (&crypt_prov, 0, 0, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);

   return crypt_prov != 0;
}

lw_bool lw_random (char * buffer, size_t size)
{
   if (!crypt_init ())
      return lw_false;

   if (!CryptGenRandom (crypt_prov, size, (PBYTE) buffer))
      return lw_false;

   return lw_true;
}

void lw_md5 (char * output, const char * input, size_t length)
{
   HCRYPTPROV hash_prov;
   DWORD hash_length = 16;

   crypt_init ();

   CryptCreateHash (crypt_prov, CALG_MD5, 0, 0, &hash_prov);
   CryptHashData (hash_prov, (BYTE *) input, length, 0);
   CryptGetHashParam (hash_prov, HP_HASHVAL, (BYTE *) output, &hash_length, 0);
   CryptDestroyHash (hash_prov);
}

void lw_sha1 (char * output, const char * input, size_t length)
{
   HCRYPTPROV hash_prov;
   DWORD hash_length = 20;

   crypt_init ();

   CryptCreateHash (crypt_prov, CALG_SHA1, 0, 0, &hash_prov);
   CryptHashData (hash_prov, (BYTE *) input, length, 0);
   CryptGetHashParam (hash_prov, HP_HASHVAL, (BYTE *) output, &hash_length, 0);
   CryptDestroyHash (hash_prov);
}

WCHAR * utf8_to_wchar (const char * str)
{
   size_t src_len = strlen (str);
   size_t dest_len = MultiByteToWideChar (CP_UTF8, 0, str, src_len, 0, 0);

   WCHAR * dest = (WCHAR *) malloc (sizeof (WCHAR) * (dest_len + 1));

   dest_len = MultiByteToWideChar (CP_UTF8, 0, str, src_len, dest, dest_len);

   dest [dest_len] = 0;

   return dest;
}

