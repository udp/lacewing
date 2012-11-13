
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

#include "../lw_common.h"
#include "../Address.h"

struct Client::Internal
{
    Lacewing::Pump &Pump;

    struct
    {
        HandlerConnect     Connect;
        HandlerDisconnect  Disconnect;
        HandlerReceive     Receive;
        HandlerError       Error;

    } Handlers;

    Client &Public;

    Internal (Client &_Public, Lacewing::Pump &_Pump)
            : Public (_Public), Pump (_Pump)
    {
        Connected = Connecting = false;

        Address =  0;
        Socket  = -1;

        memset (&Handlers, 0, sizeof (Handlers));

        Address = 0;
    }

    ~ Internal ()
    {
        delete Address;
    }

    Lacewing::Address * Address;

    int Socket;
    bool Connected, Connecting;

    void WriteReady ();

    Pump::Watch * Watch;
};

Client::Client (Lacewing::Pump &Pump) : FDStream (Pump)
{
    lwp_init ();

    internal = new Internal (*this, Pump);
}

Client::~Client ()
{
    Close ();

    delete internal;
}

void Client::Connect (const char * Host, int Port)
{
    Address Address (Host, Port, true);
    Connect (Address);
}

void Client::Internal::WriteReady ()
{
    assert (Connecting);

    int Error;
    
    {   socklen_t ErrorLength = sizeof (Error);
        getsockopt (Socket, SOL_SOCKET, SO_ERROR, &Error, &ErrorLength);
    }

    if(Error != 0)
    {
        /* Failed to connect */

        Connecting = false;

        Lacewing::Error Error;
        Error.Add("Error connecting");

        if (Handlers.Error)
            Handlers.Error (Public, Error);

        return;
    }

    Public.SetFD (Socket, Watch, true);

    Connecting = false;

    if (Handlers.Connect)
        Handlers.Connect (Public);

    if (Handlers.Receive)
        Public.Read (-1);
}

static void WriteReady (Client::Internal * internal)
{   
    internal->WriteReady ();
}

void Client::Connect (Address &Address)
{
    if (Connected () || Connecting ())
    {
        Lacewing::Error Error;
        Error.Add("Already connected to a server");
        
        if (internal->Handlers.Error)
            internal->Handlers.Error (*this, Error);

        return;
    }

    internal->Connecting = true;

    /* TODO : Resolve asynchronously? */

    {   Error * Error = Address.Resolve ();

        if (Error)
        {
            if (internal->Handlers.Error)
                internal->Handlers.Error (*this, *Error);
                
            return;
        }
    }

    delete internal->Address;
    internal->Address = new Lacewing::Address (Address);

    if ((internal->Socket = socket (Address.IPv6 () ? AF_INET6 : AF_INET,
                                        SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
        Lacewing::Error Error;
       
        Error.Add(errno);        
        Error.Add("Error creating socket");
        
        if (internal->Handlers.Error)
            internal->Handlers.Error (*this, Error);

        return;
    }

    if (!Address.internal->Info)
    {
        Lacewing::Error Error;
       
        Error.Add("The provided Address object is not ready for use");
        
        if (internal->Handlers.Error)
            internal->Handlers.Error (*this, Error);

        return;
    }

    fcntl (internal->Socket, F_SETFL,
            fcntl (internal->Socket, F_GETFL, 0) | O_NONBLOCK);

    internal->Watch = internal->Pump.Add (internal->Socket, internal,
                            0, (Pump::Callback) ::WriteReady);

    if (connect (internal->Socket, Address.internal->Info->ai_addr,
                        Address.internal->Info->ai_addrlen) == -1)
    {
        if(errno == EINPROGRESS)
            return;

        internal->Connecting = false;

        Lacewing::Error Error;
       
        Error.Add(errno);        
        Error.Add("Error connecting");

        if (internal->Handlers.Error)
            internal->Handlers.Error (*this, Error);
    }
}

bool Client::Connected ()
{
    return Valid ();
}

bool Client::Connecting ()
{
    return internal->Connecting;
}

Address &Client::ServerAddress ()
{
    return *internal->Address;
}

static void onData (Stream &, void * tag, const char * buffer, size_t size)
{
    Client::Internal * internal = (Client::Internal *) tag;

    internal->Handlers.Receive (internal->Public, buffer, size);
}

void Client::onReceive (Client::HandlerReceive onReceive)
{
    internal->Handlers.Receive = onReceive;

    if (onReceive)
    {
        AddHandlerData (::onData, internal);
        Read (-1);
    }
    else
    {
        RemoveHandlerData (::onData, internal);
    }
}

static void onClose (Stream &, void * tag)
{
    Client::Internal * internal = (Client::Internal *) tag;

    internal->Handlers.Disconnect (internal->Public);
}

void Client::onDisconnect (Client::HandlerDisconnect onDisconnect)
{
    internal->Handlers.Disconnect = onDisconnect;

    if (onDisconnect)
        AddHandlerClose (::onClose, internal);
    else
        RemoveHandlerClose (::onClose, internal);
}

AutoHandlerFunctions (Client, Connect)
AutoHandlerFunctions (Client, Error)

