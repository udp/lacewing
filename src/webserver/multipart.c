
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

static lw_bool parse_disposition (lwp_ws_multipart ctx,
                                  size_t length,
                                  const char * disposition)
{
   lwp_nvhash_clear (&ctx->disposition);

   enum
   {
      s_type,
      s_param_name,
      s_param_value_start,
      s_param_value,
      s_param_value_end

   } state = s_type;

   size_t name_begin = 0, value_begin = 0, name_len;

   for (size_t i = 0; i < length; ++ i)
   {
      char c = disposition [i];

      switch (state)
      {
         case s_type:

            if (c == ';' || !c)
            {
               lwp_nvhash_set_ex
                  (&ctx->disposition, 4, "type", i, disposition, lw_true);

               if (!c)
                  return lw_true;

               state = s_param_name;

               continue;
            }

            continue;

         case s_param_value_end:

            if (!c)
               return lw_true;

            state = s_param_name;

            if (c == ';')
               continue;
            else
               return lw_false;

         /* fallthrough */
         case s_param_name:

            if (!c)
               return lw_true;

            if (!name_begin)
            {
               if (isspace (c))
                  continue;

               name_begin = i;
            }

            if (c == '=')
            {
               name_len = i - name_begin;
               state = s_param_value_start;

               continue;
            }

            continue;

         case s_param_value_start:

            state = s_param_value;

            if (c == '"')
               continue;

         /* fallthrough */
         case s_param_value:

            if (!value_begin)
               value_begin = i;

            if (c == '"' || c == ';' || !c)
            {
               lwp_nvhash_set_ex
                  (&ctx->disposition, name_len, disposition + name_begin,
                   i - value_begin, disposition + value_begin, lw_true);

               if (!c)
                  return lw_true;

               name_begin = 0;
               value_begin = 0;

               state = (c == '"') ? s_param_value_end : s_param_name;

               continue;
            }

            continue;
      };
   }

   return lw_true;
}

static int on_header_field (multipart_parser * parser,
                            const char * at,
                            size_t length)
{
   lwp_ws_multipart ctx = (lwp_ws_multipart) multipart_parser_get_data (parser);

   ctx->cur_header_name = at;
   ctx->cur_header_name_length = length;

   return 0;
}

static int on_header_value (multipart_parser * parser,
                            const char * at,
                            size_t length)
{
   lwp_ws_multipart ctx = (lwp_ws_multipart) multipart_parser_get_data (parser);

   struct _lw_ws_upload_hdr header;

   if (! (header.name = (char *) malloc (ctx->cur_header_name_length + 1)))
      return -1;

   if (! (header.value = (char *) malloc (length + 1)))
      return -1;

   memcpy (header.name, ctx->cur_header_name, ctx->cur_header_name_length);
   header.name [ctx->cur_header_name_length] = 0;

   lwp_to_lowercase (header.name);

   memcpy (header.value, at, length);
   header.value [length] = 0;

   lwp_trace ("Multipart %p: Got header: %s => %s", ctx, header.name, header.value);

   list_push (ctx->headers, header);

   if (!strcmp (header.name, "content-disposition"))
   {
      if (!parse_disposition (ctx, length, at))
         return -1;

      if (ctx->child)
      {
         lwp_nvhash_set (&ctx->child->disposition, "name",
                         lwp_nvhash_get (&ctx->disposition, "name", ""),
                         lw_true);
      }
   }
   else if (!strcmp (header.name, "content-type"))
   {
      if (lwp_begins_with (header.value, "multipart"))
      {
         ctx->child = lwp_ws_multipart_new (ctx->ws, ctx->request, header.value);

         const char * name = lwp_nvhash_get (&ctx->disposition, "name", 0);

         if (name)
            lwp_nvhash_set (&ctx->child->disposition, "name", name, lw_true);
      }
   }

   return 0;
}

static int on_headers_complete (multipart_parser * parser)
{
   lwp_ws_multipart ctx = (lwp_ws_multipart) multipart_parser_get_data (parser);

   lwp_trace ("Multipart %p: on_headers_complete", ctx);

   ctx->parsing_headers = lw_false;
   lwp_heapbuffer_reset (&ctx->request->buffer);

   if (lwp_nvhash_get (&ctx->disposition, "filename", 0))
   {
      /* A filename was given - assign this part an upload structure. */

      ctx->cur_upload = lwp_ws_upload_new (ctx->request);

      ctx->cur_upload->disposition = ctx->disposition;
      ctx->disposition = 0;

      list_each (ctx->headers, header)
      {
         list_push (ctx->cur_upload->headers, header);
      }

      list_clear (ctx->headers);

      lwp_trace ("Multipart %p: Calling on_upload_start", ctx);

      if (ctx->ws->on_upload_start)
         ctx->ws->on_upload_start (ctx->ws, ctx->request, ctx->cur_upload);
   }

   return 0;
}

static int on_part_data (multipart_parser * parser,
                         const char * at,
                         size_t length)
{
   lwp_ws_multipart ctx = (lwp_ws_multipart) multipart_parser_get_data (parser);

   if (ctx->child)
   {
      if (lwp_ws_multipart_process (ctx->child, at, length) != length)
      {
         lwp_ws_multipart_delete (ctx->child);
         ctx->child = 0;

         return -1;
      }

      if (ctx->child->done)
      {
         lwp_ws_multipart_delete (ctx->child);
         ctx->child = 0;

         return 0;
      }
   }

   if (!ctx->cur_upload)
   {
      /* No upload structure: this will be treated as a normal POST item, and
       * so the data must be buffered.
       */

      lwp_heapbuffer_add (&ctx->request->buffer, at, length);
      return 0;
   }

   if (ctx->cur_upload->autosave_file)
   {
      /* Auto save mode */

      lw_stream_write ((lw_stream) ctx->cur_upload->autosave_file, at, length);
      return 0;
   }

   /* Manual save mode */

   if (ctx->ws->on_upload_chunk)
   {
      ctx->ws->on_upload_chunk (ctx->ws, ctx->request,
                                ctx->cur_upload, at, length);
   }

   return 0;
}

static int on_part_data_begin (multipart_parser * parser)
{
   lwp_ws_multipart ctx = (lwp_ws_multipart) multipart_parser_get_data (parser);

   lwp_trace ("Multipart %p: onPartDataBegin", ctx);

   return 0;
}

static void add_upload (lwp_ws_multipart ctx, lw_ws_upload upload)
{
   ctx->uploads = (lw_ws_upload *) realloc
      (ctx->uploads, sizeof (lw_ws_upload) * (++ ctx->num_uploads));

   ctx->uploads [ctx->num_uploads - 1] = upload;
}

static int on_part_data_end (multipart_parser * parser)
{
   lwp_ws_multipart ctx = (lwp_ws_multipart) multipart_parser_get_data (parser);

   lwp_trace ("Multipart %p: on_part_data_end", ctx);

   ctx->parsing_headers = lw_true;

   if (ctx->cur_upload)
   {
      if (ctx->parent)
         add_upload (ctx->parent, ctx->cur_upload);
      else
         add_upload (ctx, ctx->cur_upload);

      if (ctx->cur_upload->autosave_file)
      {
         /* Auto save */

         lwp_trace("Closing auto save file");

         lw_stream_close ((lw_stream) ctx->cur_upload->autosave_file, lw_false);
      }
      else
      {
         /* Manual save */

         if (ctx->ws->on_upload_done)
            ctx->ws->on_upload_done (ctx->ws, ctx->request, ctx->cur_upload);
      }

      ctx->cur_upload = 0;
   }
   else
   {
      /* No upload structure - add to POST items */

      lwp_heapbuffer_add (&ctx->request->buffer, "\0", 1);

      lwp_nvhash_set (&ctx->request->post_items,
                      lwp_nvhash_get (&ctx->disposition, "name", ""),
                      lwp_heapbuffer_buffer (&ctx->request->buffer),
                      lw_true);

      lwp_heapbuffer_reset (&ctx->request->buffer);
   }

   lwp_nvhash_clear (&ctx->disposition);

   return 0;
}

static int on_body_end (multipart_parser * parser)
{
   lwp_ws_multipart ctx = (lwp_ws_multipart) multipart_parser_get_data (parser);

   lwp_trace ("Multipart %p: onBodyEnd", ctx);

   ctx->done = lw_true;

   if (!ctx->parent)
      lwp_ws_multipart_call_hook (ctx);

   return 0;
}

void lwp_ws_multipart_call_hook (lwp_ws_multipart ctx)
{
   /* The handler might have already been called in autosave mode
    */
   if (ctx->called_handler)
      return;

   /* Only call the handler if all files are closed
    */
   for (int i = 0; i < ctx->num_uploads; ++ i)
   {
      if (ctx->uploads [i]->autosave_file)
         return;
   }

   ctx->called_handler = lw_true;

   lwp_ws_req_before_handler (ctx->request);

   if (ctx->ws->on_upload_post)
   {
      ctx->ws->on_upload_post (ctx->ws, ctx->request,
                               ctx->uploads, ctx->num_uploads);
   }

   lwp_ws_req_after_handler (ctx->request);
}
    
const multipart_parser_settings settings =
{
   on_header_field,
   on_header_value,
   on_part_data,
   on_part_data_begin,
   on_headers_complete,
   on_part_data_end,
   on_body_end
};

lwp_ws_multipart lwp_ws_multipart_new (lw_ws ws, lw_ws_req request,
                                       const char * content_type)
{
   lwp_ws_multipart ctx = (lwp_ws_multipart) calloc (sizeof (*ctx), 1);

   if (!ctx)
      return 0;

   ctx->ws = ws;
   ctx->request = request;

   const char * _boundary = strstr (content_type, "boundary=") + 9;

   char * boundary = (char *) alloca (strlen (_boundary) + 3);

   strcpy (boundary, "--");
   strcat (boundary, _boundary);

   lwp_trace ("Creating parser with boundary: %s", boundary);

   ctx->parser = multipart_parser_init (boundary, &settings);
   multipart_parser_set_data (ctx->parser, ctx);

   ctx->parsing_headers = lw_true;

   return ctx;
}

void lwp_ws_multipart_delete (lwp_ws_multipart ctx)
{
   multipart_parser_free (ctx->parser);

   lwp_nvhash_clear (&ctx->disposition);

   list_each (ctx->headers, header)
   {
      free (header.name);
      free (header.value);
   }

   list_clear (ctx->headers);

   if (ctx->child)
      lwp_ws_multipart_delete (ctx->child);

   free (ctx->uploads);

   free (ctx);
}

size_t lwp_ws_multipart_process (lwp_ws_multipart ctx,
                                 const char * buffer,
                                 size_t buffer_size)
{
   size_t size = buffer_size;

   assert (size != 0);

   /* TODO : This code is duplicated in http-parse.c for HTTP headers */

   if (ctx->parsing_headers)
   {
      for (size_t i = 0; i < size; )
      {
         {  char b = buffer [i];

            if (b == '\r')
            {
               if (buffer [i + 1] == '\n')
                  ++ i;
            }
            else if (b != '\n')
            {
               ++ i;
               continue;
            }
         }

         int to_parse = i + 1;
         lw_bool error = lw_false;

         if (lwp_heapbuffer_length (&ctx->request->buffer) > 0)
         {
            lwp_heapbuffer_add (&ctx->request->buffer, buffer, to_parse);

            size_t parsed = multipart_parser_execute
               (ctx->parser, lwp_heapbuffer_buffer (&ctx->request->buffer),
                             lwp_heapbuffer_length (&ctx->request->buffer));

            if (parsed != lwp_heapbuffer_length (&ctx->request->buffer))
               error = lw_true;

            lwp_heapbuffer_reset (&ctx->request->buffer);
         }
         else
         {
            size_t parsed = multipart_parser_execute
               (ctx->parser, buffer, to_parse);

            if (parsed != to_parse)
               error = lw_true;
         }

         size -= to_parse;
         buffer += to_parse;

         if (error)
         {
            lwp_trace ("Multipart error");
            return 0;
         }

         if (!ctx->parsing_headers)
            break;

         i = 0;
      }

      if (ctx->parsing_headers)
      {
         /* TODO : max line length */

         lwp_heapbuffer_add (&ctx->request->buffer, buffer, size);
         return buffer_size;
      }
      else
      {
         if (!size)
            return buffer_size;
      }
   }

   size_t parsed = multipart_parser_execute (ctx->parser, buffer, size);

   if (parsed != size)
      return 0;

   return buffer_size;
}



