
/* vim: set et ts=4 sw=4 ft=cpp:
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

#include "../lw_common.h"
#include "WinSSLClient.h"

WinSSLClient::Downstream::Downstream (WinSSLClient &_Client)
    : Client (_Client)
{
}

WinSSLClient::Upstream::Upstream (WinSSLClient &_Client)
    : Client (_Client)
{
}

WinSSLClient::WinSSLClient (CredHandle server_creds)
    : ServerCreds (server_creds), Downstream (*this), Upstream (*this)
{
    Flags = 0;
    Status = SEC_I_CONTINUE_NEEDED;
}

WinSSLClient::~ WinSSLClient ()
{
}

size_t WinSSLClient::Upstream::Put (const char * buffer, size_t size)
{
    if (! (Client.Flags & Flag_HandshakeComplete))
        return 0; /* can't send anything right now */

    SecBuffer buffers [4];

        buffers [0].pvBuffer = (void *) alloca (Client.Sizes.cbHeader);
        buffers [0].cbBuffer = Client.Sizes.cbHeader;
        buffers [0].BufferType = SECBUFFER_STREAM_HEADER;

        buffers [1].pvBuffer = (void *) buffer;
        buffers [1].cbBuffer = size;
        buffers [1].BufferType = SECBUFFER_DATA;

        buffers [2].pvBuffer = (void *) alloca (Client.Sizes.cbTrailer);
        buffers [2].cbBuffer = Client.Sizes.cbTrailer;
        buffers [2].BufferType = SECBUFFER_STREAM_TRAILER;

        buffers [3].BufferType = SECBUFFER_EMPTY;
        buffers [3].cbBuffer = 0;

    SecBufferDesc buffers_desc;

        buffers_desc.cBuffers = 4;
        buffers_desc.pBuffers = buffers;
        buffers_desc.ulVersion = SECBUFFER_VERSION;

    SECURITY_STATUS Status = EncryptMessage
        (&Client.Context, 0, &buffers_desc, 0);

    if (Status != SEC_E_OK)
    {
        /* TODO : error? */

        return size;
    }

    Data ((char *) buffers [0].pvBuffer, buffers [0].cbBuffer);
    Data ((char *) buffers [1].pvBuffer, buffers [1].cbBuffer);
    Data ((char *) buffers [2].pvBuffer, buffers [2].cbBuffer);
    Data ((char *) buffers [3].pvBuffer, buffers [3].cbBuffer);

    return size;
}

size_t WinSSLClient::Downstream::Put (const char * buffer, size_t size)
{
    size_t processed = 0;

    if (! (Client.Flags & Flag_HandshakeComplete))
    {
        processed += ProcHandshakeData (buffer, size);

        if (! (Client.Flags & Flag_HandshakeComplete))
            return processed;

        /* Handshake is complete now */

        buffer += processed;
        size -= processed;
    }

    processed += ProcMessageData (buffer, size);

    return processed;
}

size_t WinSSLClient::Downstream::ProcHandshakeData (const char * buffer, size_t size)
{
    SecBuffer in [2];

        in [0].BufferType = SECBUFFER_TOKEN;
        in [0].pvBuffer = (void *) buffer;
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

    SecBufferDesc in_desc, out_desc;

        in_desc.ulVersion = SECBUFFER_VERSION;
        in_desc.pBuffers = in;
        in_desc.cBuffers = 2;

        out_desc.ulVersion = SECBUFFER_VERSION;
        out_desc.pBuffers = out;
        out_desc.cBuffers = 2;

    int flags = ASC_REQ_SEQUENCE_DETECT | ASC_REQ_REPLAY_DETECT |
                ASC_REQ_CONFIDENTIALITY | ASC_REQ_EXTENDED_ERROR |
                ASC_REQ_ALLOCATE_MEMORY | ASC_REQ_STREAM;

    unsigned long out_flags;
    TimeStamp expiry_time;

    Client.Status = AcceptSecurityContext
    (
        &Client.ServerCreds,
        Client.Flags & Flag_GotContext ? &Client.Context : 0,
        &in_desc,
        flags,
        SECURITY_NATIVE_DREP,
        Client.Flags & Flag_GotContext ? 0 : &Client.Context,
        &out_desc,
        &out_flags,
        &expiry_time
    );

    Client.Flags |= Flag_GotContext;

    if (FAILED (Client.Status))
    {
        if (Client.Status == SEC_E_INCOMPLETE_MESSAGE)
            return 0; /* need more data */

        /* Lacewing::Error Error;
        
        Error.Add(WSAGetLastError ());
        Error.Add("Secure handshake failure");
        
        if (Client.Server.Handlers.Error)
            Client.Server.Handlers.Error(Client.Server.Public, Error);

        Client.Public.Disconnect(); */

        return size;
    }

    if (Client.Status == SEC_E_OK
            || Client.Status == SEC_I_CONTINUE_NEEDED)
    {
        /* Did AcceptSecurityContext give us back a response to send? */

        if (out [0].cbBuffer && out [0].pvBuffer)
        {
            Client.Upstream.Data ((char *) out [0].pvBuffer, out [0].cbBuffer);

            FreeContextBuffer (out [0].pvBuffer);
        }

        /* Is there any data left over? */

        if (in [1].BufferType == SECBUFFER_EXTRA)
            size -= in [1].cbBuffer;

        if (Client.Status == SEC_E_OK)
        {
            /* Handshake complete!  Find out the maximum message size and
             * how big the header/trailer will be.
             */
            
            if ((Client.Status = QueryContextAttributes (&Client.Context,
                    SECPKG_ATTR_STREAM_SIZES, &Client.Sizes)) != SEC_E_OK)
            {
                /* Lacewing::Error Error;
                
                Error.Add(WSAGetLastError ());
                Error.Add("Secure handshake failure");

                if (Client.Server.Handlers.Error)
                  Client.Server.Handlers.Error (Client.Server.Public, Error);

                Client.Public.Disconnect(); */

                return size;
            }

            Client.Flags |= Flag_HandshakeComplete;
        }
    }

    return size;
}

size_t WinSSLClient::Downstream::ProcMessageData (const char * buffer, size_t size)
{
    SecBuffer buffers [4];

        buffers [0].pvBuffer = (void *) buffer;
        buffers [0].cbBuffer = size;
        buffers [0].BufferType = SECBUFFER_DATA;

        buffers [1].BufferType = SECBUFFER_EMPTY;
        buffers [2].BufferType = SECBUFFER_EMPTY;
        buffers [3].BufferType = SECBUFFER_EMPTY;

    SecBufferDesc buffers_desc;

        buffers_desc.cBuffers = 4;
        buffers_desc.pBuffers = buffers;
        buffers_desc.ulVersion = SECBUFFER_VERSION;

    Client.Status = DecryptMessage (&Client.Context, &buffers_desc, 0, 0);

    if (Client.Status == SEC_E_INCOMPLETE_MESSAGE)
        return size; /* need more data */
    
    if (Client.Status ==
            _HRESULT_TYPEDEF_ (0x00090317L)) /* SEC_I_CONTENT_EXPIRED */
    {
        /* Client.Public.Disconnect(); */
        return size;
    }
 
    if (Client.Status == SEC_I_RENEGOTIATE)
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

    if (FAILED (Client.Status))
    {
        /* Error decrypting the message */

        /* Lacewing::Error Error;
        Error.Add(Status);
        lwp_trace("Error decrypting the message: %s", Error.ToString ());

        Client.Public.Disconnect(); */

        return size;
    }

     /* Find the decrypted data */

    for (int i = 0; i < 4; ++ i)
    {
        SecBuffer &buffer = buffers [i];

        if (buffer.BufferType == SECBUFFER_DATA)
        {
            Data ((char *) buffer.pvBuffer, buffer.cbBuffer);
            break;
        }
    }

    /* Check for any trailing data that wasn't part of the message */

    for (int i = 0; i < 4; ++ i)
    {
        SecBuffer &buffer = buffers [i];

        if (buffer.BufferType == SECBUFFER_EXTRA && buffer.cbBuffer > 0)
        {
            size -= buffer.cbBuffer;
            break;
        }
    }

    return size;
}




