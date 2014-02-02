
/* vim: set et ts=3 sw=3 ft=c:
 *
 * Copyright (C) 2012 James McLaughlin.  All rights reserved.
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

static void client_respond (lwp_ws_client, lw_ws_req request);
static void client_tick (lwp_ws_client);
static void client_cleanup (lwp_ws_client);

lwp_ws_client lwp_ws_httpclient_new (lw_ws ws, lw_server_client socket,
                                     lw_bool secure)
{
   lwp_ws_httpclient ctx = (lwp_ws_httpclient) calloc (sizeof (*ctx), 1);

   if (!ctx)
      return 0;

   ctx->client.ws = ws;
   ctx->client.socket = socket;
   
   ctx->client.respond  = client_respond;
   ctx->client.tick     = client_tick;
   ctx->client.cleanup  = client_cleanup;

   lwp_stream_init ((lw_stream) ctx, &def_httpclient, 0);

   ctx->request = lwp_ws_req_new (ws, (lwp_ws_client) ctx, &def_httprequest);

   http_parser_init (&ctx->parser, HTTP_REQUEST);
   ctx->parser.data = ctx;

   ctx->parsing_headers = lw_true;
   ctx->signal_eof = lw_false;

   lw_stream_write_stream
      ((lw_stream) socket, (lw_stream) ctx->request, -1, lw_false);

   lw_stream_begin_queue ((lw_stream) ctx->request);

   /* When the retry mode is more_data and we can't sink everything, our sink
    * method will be called again as soon as more data arrives.
    */

   lw_stream_retry ((lw_stream) ctx, lw_stream_retry_more_data);
   
   return (lwp_ws_client) ctx;
}

void client_cleanup (lwp_ws_client client)
{
   lwp_ws_httpclient ctx = (lwp_ws_httpclient) client;

   /* Only call the disconnect handler for requests that have not yet been
    * completed (responded == false)
    */

   if (!ctx->request->responded)
   {
      if (ctx->client.ws->on_disconnect)
         ctx->client.ws->on_disconnect (ctx->client.ws, ctx->request);
   }

   lwp_ws_req_delete (ctx->request);
}



/*
 * Stream implementation
 */

static size_t def_sink_data (lw_stream stream, const char * buffer, size_t size)
{
   lwp_ws_httpclient ctx = (lwp_ws_httpclient) stream;

   lwp_trace ("HTTP got " lwp_fmt_size " bytes", size);
    
   size_t processed = 0;

   /* TODO: A naughty client could keep the connection open by sending 1 byte
    * every 5 seconds.
    */

   ctx->last_activity = time (0);

   for (;;)
   {
      if (!ctx->request->responded)
      {
         /* The application hasn't yet called Finish() for the last request,
          * so no more data can be processed.
          */

         return processed;
      }

      /* Already processed everything in the last loop iteration? */

      if (processed == size)
         return processed;

      /* When parsing headers, we provide the HTTP parser with complete lines
       * to avoid getting any fragmented data.
       *
       * When partial lines are received, we can take advantage of the
       * natural buffering provided by ws_httpclient being a stream (by
       * returning < size bytes from sink and calling stream_retry later).
       */

      /* TODO: max line length */

      if (ctx->parsing_headers)
      {
         for (size_t i = processed; i < size; ++ i)
         {
            char b = buffer [i];

            if (b == '\r' || b == '\n')
            {
               /* Reached the end of a line */

               size_t to_parse = (i + 1) - processed;

               size_t parsed = http_parser_execute (&ctx->parser,
                                                    &parser_settings,
                                                    buffer + processed,
                                                    to_parse);

               processed += parsed;

               if (ctx->parser.http_errno == HPE_PAUSED)
               {
                  /* Paused by onMessageEnd - this has no significance, since
                   * we're already parsing headers.
                   */

                  http_parser_pause (&ctx->parser, 0);
                  continue;
               }
               else if (parsed != to_parse || ctx->parser.upgrade)
               {
                  lwp_trace ("HTTP error (headers), closing socket...");

                  lw_stream_close ((lw_stream) ctx->client.socket, lw_true);
                  return size;
               }

               if (!ctx->parsing_headers)
                  break;  /* headers finished */
            }
         }

         /* Reached the end of the buffer - are we still parsing headers? */

         if (ctx->parsing_headers)
            return processed;
         else
         {
            if (processed == size)
               return processed;
         }
      }

      /* Parsing the body.
       *
       * Note that we have no idea if the buffer we have here contains only
       * body data - it might well contain a fragment of the next request,
       * which we would want to pass to the HTTP parser line by line.  For
       * this reason, the parser will always be paused in on_message_complete
       * so that control is returned here.
       */

      size_t to_parse = size - processed;

      size_t parsed = http_parser_execute (&ctx->parser, &parser_settings,
                                           buffer + processed,
                                           to_parse);

      processed += parsed;

      if (ctx->parser.http_errno == HPE_PAUSED)
      {
         /* on_message_complete always pauses the parser so that control
          * is returned here, so that we can transition back to parsing
          * headers.
          */

         http_parser_pause (&ctx->parser, 0);
      }
      else if (parsed != to_parse || ctx->parser.upgrade)
      {
         lwp_trace ("HTTP error (body), closing socket...");

         lw_stream_close ((lw_stream) ctx->client.socket, lw_true);
         return size;
      }

      if (ctx->signal_eof)
      {
         http_parser_execute (&ctx->parser, &parser_settings, 0, 0);
         ctx->signal_eof = lw_false;
      }

      return processed;
   }
}

const lw_streamdef def_httpclient =
{
   def_sink_data,
   0, /* sink_stream */
   0, /* retry */
   0, /* is_transparent */
   0, /* close */
   0, /* bytes_left */
   0, /* read */
   0  /* cleanup */
};

static lw_bool def_is_transparent (lw_stream stream)
{
   return lw_true;
}

const lw_streamdef def_httprequest =
{
   0, /* sink_data */
   0, /* sink_stream */
   0, /* retry */
   def_is_transparent,
   0, /* close */
   0, /* bytes_left */
   0, /* read */
   0  /* cleanup */
};

void client_respond (lwp_ws_client client, lw_ws_req request)
{ 
   lwp_ws_httpclient ctx = (lwp_ws_httpclient) client;

   /* The request parameter is redundant here, because HTTP only ever has one
    * request object per client (unlike SPDY).  Sanity check...
    */

   assert (request == ctx->request);

   ctx->last_activity = time (0);

   /* TODO: Eliminate the use of this buffer (use stream queueing instead) */

   lwp_heapbuffer_reset (&request->buffer);

   lwp_heapbuffer_addf (&request->buffer, "HTTP/%d.%d %s",
                        (int) request->version_major,
                        (int) request->version_minor,
                        request->status);

   list_each (request->headers_out, header)
   {
      lwp_heapbuffer_addf (&request->buffer, "\r\n%s: %s",
                           header.name, header.value);
   }

   for (lw_ws_req_cookie cookie = request->cookies; cookie;
         cookie = (lw_ws_req_cookie) cookie->hh.next)
   {
      if (!cookie->changed)
         continue;

      lwp_heapbuffer_addf (&request->buffer, "\r\nset-cookie: %s=%s",
                             cookie->name, cookie->value);

      if (*cookie->attr)
         lwp_heapbuffer_addf (&request->buffer, "; %s", cookie->attr);
   }

   lwp_heapbuffer_addf (&request->buffer, "\r\ncontent-length: " lwp_fmt_size "\r\n\r\n",
                           lw_stream_queued ((lw_stream) ctx->request));

   lw_fdstream_cork ((lw_fdstream) ctx->client.socket);

   char * head_buffer = lwp_heapbuffer_buffer (&request->buffer);
   size_t head_length = lwp_heapbuffer_length (&request->buffer);

   lw_stream_end_queue_hb ((lw_stream) ctx->request, 1,
                           (const char **) &head_buffer, &head_length);

   lw_stream_begin_queue ((lw_stream) ctx->request);

   lwp_heapbuffer_reset (&request->buffer);

   lw_fdstream_uncork ((lw_fdstream) ctx->client.socket);

   if (!http_should_keep_alive (&ctx->parser))
      lw_stream_close ((lw_stream) ctx->client.socket, lw_false);

   request->responded = lw_true;

   /* If any data was queued while waiting to respond to this request, we'll
    * be able to process it now.
    */

   lw_stream_retry ((lw_stream) ctx, lw_stream_retry_now);
}

void client_tick (lwp_ws_client client)
{
   lwp_ws_httpclient ctx = (lwp_ws_httpclient) client;

   if (ctx->request->responded
         && (time(0) - ctx->last_activity) > ctx->client.ws->timeout)
   {
      lwp_trace ("Dropping HTTP connection due to inactivity (%s)",
            lw_addr_tostring (lw_server_client_addr (ctx->client.socket)));

      lw_stream_close ((lw_stream) ctx->client.socket, lw_true);
   }
}

