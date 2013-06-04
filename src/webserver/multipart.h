
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

typedef struct _lwp_ws_multipart
{
   lw_ws ws;
   lw_ws_req request;

   struct _lwp_ws_multipart * parent, * child;

   multipart_parser * parser;

   lw_bool done;
   lw_bool called_handler;

   lwp_nvhash disposition;

   lw_bool parsing_headers;

   const char * cur_header_name;
   size_t cur_header_name_length;

   list (struct _lw_ws_upload_hdr, headers);

   lw_ws_upload cur_upload;

   lw_ws_upload * uploads;
   int num_uploads;

} * lwp_ws_multipart;

lwp_ws_multipart lwp_ws_multipart_new
    (lw_ws, lw_ws_req, const char * content_type);

void lwp_ws_multipart_delete (lwp_ws_multipart);

size_t lwp_ws_multipart_process
    (lwp_ws_multipart, const char * buffer, size_t size);

void lwp_ws_multipart_call_hook (lwp_ws_multipart);

