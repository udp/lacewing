
/* vim: set et ts=4 sw=4 ft=cpp:
 *
 * Copyright (C) 2011, 2012 James McLaughlin.  All rights reserved.
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

struct FlashPolicy::Internal
{
    char * Buffer;
    size_t Size;

    FlashPolicy &Public;
    Server Socket;

    struct
    {
        HandlerError Error;

    } Handlers;

    Internal (FlashPolicy &_Public, Pump &_Pump)
            : Public (_Public), Socket (_Pump)
    {
        memset (&Handlers, 0, sizeof (Handlers));

        Socket.Tag = this;

        Buffer = 0;
    }
};

void SocketReceive (Server &server, Server::Client &client,
                    const char * buffer, size_t size)
{
    FlashPolicy::Internal * internal = (FlashPolicy::Internal *) server.Tag;

    for (size_t i = 0; i < size; ++i)
    {
        if (!buffer [i])
        {
            client.Write (internal->Buffer, internal->Size);
            client.Write ("\0", 1);

            return;
        }
    }
}

void SocketError (Server &Socket, Error &Error)
{
    FlashPolicy::Internal * internal = (FlashPolicy::Internal *) Socket.Tag;

    Error.Add("Socket error");
    
    if (internal->Handlers.Error)
        internal->Handlers.Error (internal->Public, Error);
}

FlashPolicy::FlashPolicy (Pump &Pump)
{
    internal = new FlashPolicy::Internal (*this, Pump);

    internal->Socket.onError   (SocketError);
    internal->Socket.onReceive (SocketReceive);
}

FlashPolicy::~FlashPolicy ()
{
    Unhost();
    
    delete internal;
}

void FlashPolicy::Host (const char * Filename)
{
    Filter Filter;
    Host (Filename, Filter);
}

void FlashPolicy::Host (const char * Filename, Filter &Filter)
{
    Unhost();

    Filter.LocalPort (843);
    
    {   FILE * File = fopen(Filename, "r");

        if(!File)
        {
            Lacewing::Error Error;

            Error.Add (lwp_last_socket_error);
            Error.Add ("Error opening file: %s", Filename);
                
            if (internal->Handlers.Error)
                internal->Handlers.Error(*this, Error);

            return;
        }

        fseek(File, 0, SEEK_END);

        internal->Size = ftell(File);
        internal->Buffer = (char *) malloc(internal->Size);
        
        fseek(File, 0, SEEK_SET);

        size_t bytes = fread (internal->Buffer, 1, internal->Size, File);
        
        if (bytes != internal->Size)
        {
            internal->Size = bytes;

            if (ferror (File))
            {
                Lacewing::Error Error;
                
                Error.Add (lwp_last_error);
                Error.Add ("Error reading file: %s", Filename);

                if (internal->Handlers.Error)
                    internal->Handlers.Error (*this, Error);

                free (internal->Buffer);
                internal->Buffer = 0;
        
                fclose (File);
                
                return;
            }
        }

        fclose (File);
    }

    internal->Socket.Host (Filter);
}

void FlashPolicy::Unhost ()
{
    internal->Socket.Unhost ();

    free (internal->Buffer);
    internal->Buffer = 0;
}

bool FlashPolicy::Hosting ()
{
    return internal->Socket.Hosting ();
}

AutoHandlerFunctions (FlashPolicy, Error);

