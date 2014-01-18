
/* vim: set et ts=3 sw=3 sts=3 ft=c:
 *
 * Copyright (C) 2013 James McLaughlin et al.  All rights reserved.
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

#include "../../common.h"
#include "ssl.h"

static size_t proc_message_data
    (lwp_ssl, const char * buffer, size_t size);

static size_t def_upstream_sink_data (lw_stream upstream,
                                      const char * buffer,
                                      size_t size)
{
   lwp_ssl ctx = container_of
      (upstream, struct _lwp_ssl, upstream);

   if (!ctx->handshake_complete)
      return 0; /* can't send anything right now */

   SecBuffer buffers [4];

      buffers [0].pvBuffer = ctx->header;
      buffers [0].cbBuffer = ctx->sizes.cbHeader;
      buffers [0].BufferType = SECBUFFER_STREAM_HEADER;

      buffers [1].pvBuffer = (BYTE *) buffer;
      buffers [1].cbBuffer = size;
      buffers [1].BufferType = SECBUFFER_DATA;

      buffers [2].pvBuffer = ctx->trailer;
      buffers [2].cbBuffer = ctx->sizes.cbTrailer;
      buffers [2].BufferType = SECBUFFER_STREAM_TRAILER;

      buffers [3].BufferType = SECBUFFER_EMPTY;
      buffers [3].cbBuffer = 0;

   SecBufferDesc buffers_desc = {};
    
   buffers_desc.cBuffers = 4;
   buffers_desc.pBuffers = buffers;
   buffers_desc.ulVersion = SECBUFFER_VERSION;

   SECURITY_STATUS status = EncryptMessage (&ctx->context, 0, &buffers_desc, 0);

   if (status != SEC_E_OK)
   {
      /* TODO : error? */

      return size;
   }

   lw_stream_data (upstream, (char *) buffers [0].pvBuffer, buffers [0].cbBuffer);
   lw_stream_data (upstream, (char *) buffers [1].pvBuffer, buffers [1].cbBuffer);
   lw_stream_data (upstream, (char *) buffers [2].pvBuffer, buffers [2].cbBuffer);
   lw_stream_data (upstream, (char *) buffers [3].pvBuffer, buffers [3].cbBuffer);

   return size;
}

static size_t def_downstream_sink_data (lw_stream downstream,
                                        const char * buffer,
                                        size_t size)
{
   lwp_ssl ctx =
      container_of (downstream, struct _lwp_ssl, downstream);

   size_t processed = 0;

   if (! (ctx->handshake_complete))
   {
      processed += ctx->proc_handshake_data (ctx, buffer, size);

      if (!ctx->handshake_complete)
         return processed;

      buffer += processed;
      size -= processed;

      /* Handshake complete!  Find out the maximum message size and
       * how big the header/trailer will be.
       */
      if ((ctx->status = QueryContextAttributes (&ctx->context,
                                                 SECPKG_ATTR_STREAM_SIZES,
                                                 &ctx->sizes)) != SEC_E_OK)
      {
         /* Lacewing::Error Error;

            Error.Add(WSAGetLastError ());
            Error.Add("Secure handshake failure");

            if (ctx->Server.Handlers.Error)
            ctx->Server.Handlers.Error (ctx->Server.Public, Error);

            ctx->Public.Disconnect(); */

         return size;
      }

      ctx->header = (char *) malloc (ctx->sizes.cbHeader);
      ctx->trailer = (char *) malloc (ctx->sizes.cbTrailer);
   }

   processed += proc_message_data (ctx, buffer, size);

   return processed;
}

size_t proc_message_data (lwp_ssl ctx, const char * buffer, size_t size)
{
   SecBuffer buffers [4];

      buffers [0].pvBuffer = (BYTE *) buffer;
      buffers [0].cbBuffer = size;
      buffers [0].BufferType = SECBUFFER_DATA;

      buffers [1].BufferType = SECBUFFER_EMPTY;
      buffers [2].BufferType = SECBUFFER_EMPTY;
      buffers [3].BufferType = SECBUFFER_EMPTY;

   SecBufferDesc buffers_desc = {};

   buffers_desc.cBuffers = 4;
   buffers_desc.pBuffers = buffers;
   buffers_desc.ulVersion = SECBUFFER_VERSION;

   ctx->status = DecryptMessage (&ctx->context, &buffers_desc, 0, 0);

   if (ctx->status == SEC_E_INCOMPLETE_MESSAGE)
      return size; /* need more data */

   if (ctx->status == _HRESULT_TYPEDEF_ (0x00090317L)) /* SEC_I_CONTENT_EXPIRED */
   {
      /* ctx->Public.Disconnect(); */
      return size;
   }

   if (ctx->status == SEC_I_RENEGOTIATE)
   {
      /* TODO: "The DecryptMessage (Schannel) function returns
       * SEC_I_RENEGOTIATE when the message sender wants to renegotiate the
       * connection (security context). An application handles a requested
       * renegotiation by calling AcceptSecurityContext (Schannel) (server
       * side) or InitializeSecurityContext (Schannel) (client side) and
       * passing in empty input buffers. After this initial call returns a
       * value, proceed as though your application were creating a new
       * connection. For more information, see Creating an Schannel Security
       * Context"
       *
       * http://msdn.microsoft.com/en-us/library/aa374781%28v=VS.85%29.aspx
       */

      return size;
   }

   if (FAILED (ctx->status))
   {
      /* Error decrypting the message */

      /* Lacewing::Error Error;
         Error.Add(Status);
         lwp_trace("Error decrypting the message: %s", Error.ToString ());

         ctx->Public.Disconnect(); */

      return size;
   }

   /* Find the decrypted data
    */
   for (int i = 0; i < 4; ++ i)
   {
      SecBuffer * buffer = (buffers + i);

      if (buffer->BufferType == SECBUFFER_DATA)
      {
         lw_stream_data (&ctx->downstream, (char *) buffer->pvBuffer, buffer->cbBuffer);
         break;
      }
   }

   /* Check for any trailing data that wasn't part of the messagei
    */
   for (int i = 0; i < 4; ++ i)
   {
      SecBuffer * buffer = (buffers + i);

      if (buffer->BufferType == SECBUFFER_EXTRA && buffer->cbBuffer > 0)
      {
         size -= buffer->cbBuffer;
         break;
      }
   }

   return size;
}

const static lw_streamdef def_upstream =
{
   def_upstream_sink_data,
   0, /* sink_stream */
   0, /* retry */
   0, /* is_transparent */
   0, /* close */
   0, /* bytes_left */
   0, /* read */
   0  /* cleanup */
};

const static lw_streamdef def_downstream =
{
   def_downstream_sink_data,
   0, /* sink_stream */
   0, /* retry */
   0, /* is_transparent */
   0, /* close */
   0, /* bytes_left */
   0, /* read */
   0  /* cleanup */
};

void lwp_ssl_init (lwp_ssl ctx, lw_stream socket)
{
   memset (ctx, 0, sizeof (*ctx));

   ctx->status = SEC_I_CONTINUE_NEEDED;

   lwp_stream_init (&ctx->upstream, &def_upstream, 0);
   lwp_stream_init (&ctx->downstream, &def_downstream, 0);

   lw_stream_add_filter_upstream
      (socket, &ctx->upstream, lw_false, lw_true);

   lw_stream_add_filter_downstream
      (socket, &ctx->downstream, lw_false, lw_true);
}

void lwp_ssl_cleanup (lwp_ssl ctx)
{
   lw_stream_close (&ctx->downstream, lw_true);
   lw_stream_close (&ctx->upstream, lw_true);

   free (ctx->header);
   free (ctx->trailer);
}

