
/* vim: set et ts=3 sw=3 sts=3 ft=c:
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

#include "../../common.h"
#include "../../stream.h"
#include "serverssl.h"

static size_t proc_handshake_data (lwp_ssl ssl, const char * buffer, size_t size);

void lwp_serverssl_init (lwp_serverssl ctx,
                         CredHandle server_creds,
                         lw_stream socket)
{
   lwp_ssl_init (&ctx->ssl, socket);

   ctx->ssl.proc_handshake_data = proc_handshake_data;
}

void lwp_serverssl_cleanup (lwp_serverssl ctx)
{
   lwp_ssl_cleanup (&ctx->ssl);
}

size_t proc_handshake_data (lwp_ssl ssl, const char * buffer, size_t size)
{
   lwp_serverssl ctx = (lwp_serverssl) ssl;

   SecBuffer in [2];

      in [0].BufferType = SECBUFFER_TOKEN;
      in [0].pvBuffer = (BYTE *) buffer;
      in [0].cbBuffer = size;

      in [1].BufferType = SECBUFFER_EMPTY;
      in [1].pvBuffer = 0;
      in [1].cbBuffer = 0;

   SecBuffer out [2];

      out [0].BufferType = SECBUFFER_TOKEN;
      out [0].pvBuffer = 0;
      out [0].cbBuffer = 0;

      out [1].BufferType = SECBUFFER_EMPTY;
      out [1].pvBuffer = 0;
      out [1].cbBuffer = 0;

   SecBufferDesc in_desc = {};

   in_desc.ulVersion = SECBUFFER_VERSION,
   in_desc.pBuffers = in;
   in_desc.cBuffers = 2;

   SecBufferDesc out_desc = {};

   out_desc.ulVersion = SECBUFFER_VERSION,
   out_desc.pBuffers = out;
   out_desc.cBuffers = 2;

   int flags = ASC_REQ_SEQUENCE_DETECT | ASC_REQ_REPLAY_DETECT |
      ASC_REQ_CONFIDENTIALITY | ASC_REQ_EXTENDED_ERROR |
      ASC_REQ_ALLOCATE_MEMORY | ASC_REQ_STREAM;

   unsigned long out_flags;
   TimeStamp expiry_time;

   ctx->ssl.status = AcceptSecurityContext
   (
      &ctx->server_creds,
      ctx->ssl.got_context ? &ctx->ssl.context : 0,
      &in_desc,
      flags,
      SECURITY_NATIVE_DREP,
      ctx->ssl.got_context ? 0 : &ctx->ssl.context,
      &out_desc,
      &out_flags,
      &expiry_time
   );

   ctx->ssl.got_context = lw_true;

   if (FAILED (ctx->ssl.status))
   {
      if (ctx->ssl.status == SEC_E_INCOMPLETE_MESSAGE)
         return 0; /* need more data */

        /* Lacewing::Error Error;
        
        Error.Add(WSAGetLastError ());
        Error.Add("Secure handshake failure");
        
        if (ctx->Server.Handlers.Error)
            ctx->Server.Handlers.Error(ctx->Server.Public, Error);

        ctx->Public.Disconnect(); */

      return size;
   }

   if (ctx->ssl.status == SEC_E_OK || ctx->ssl.status == SEC_I_CONTINUE_NEEDED)
   {
      /* Did AcceptSecurityContext give us back a response to send?
       */
      if (out [0].cbBuffer && out [0].pvBuffer)
      {
         lw_stream_data (&ctx->ssl.upstream, (char *) out [0].pvBuffer, out [0].cbBuffer);

         FreeContextBuffer (out [0].pvBuffer);
      }

      /* Is there any data left over?
       */
      if (in [1].BufferType == SECBUFFER_EXTRA)
         size -= in [1].cbBuffer;

      if (ctx->ssl.status == SEC_E_OK)
         ctx->ssl.handshake_complete = lw_true;
   }

   return size;
}

