
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
        Connecting = false;

        Socket = INVALID_HANDLE_VALUE;

        memset (&Handlers, 0, sizeof (Handlers));

        Address = 0;

        Watch = 0;
    }

    ~ Internal ()
    {
        delete Address;
    }

    Lacewing::Address * Address;

    HANDLE Socket;
    bool Connecting;

    void Completion
        (OVERLAPPED *, unsigned int BytesTransferred, int Error);

    Pump::Watch * Watch;
};

Client::Client (Lacewing::Pump &Pump) : FDStream (Pump)
{
    lwp_init ();

    internal = new Internal (*this, Pump);
}

Client::~Client ()
{
    delete internal;
}

void Client::Connect (const char * Host, int Port)
{
    Address Address (Host, Port, true);
    Connect (Address);
}

static void Completion (void * tag, OVERLAPPED * overlapped,
                            unsigned int BytesTransferred, int error)
{
    Client::Internal * internal = (Client::Internal *) tag;

    assert (internal->Connecting);

    if(error)
    {
        internal->Connecting = false;

        Lacewing::Error Error;

        Error.Add(error);
        Error.Add("Error connecting");

        if (internal->Handlers.Error)
            internal->Handlers.Error (internal->Public, Error);

        return;
    }

    internal->Public.SetFD (internal->Socket, internal->Watch, true);
    internal->Connecting = false;

    if (internal->Handlers.Connect)
        internal->Handlers.Connect (internal->Public);

    if (internal->Handlers.Receive)
        internal->Public.Read (-1);
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

    if (!Address.internal->Info)
    {
        Lacewing::Error Error;
        Error.Add ("Invalid address");
        
        if (internal->Handlers.Error)
            internal->Handlers.Error (*this, Error);

        return;
    }

    if ((internal->Socket = (HANDLE) WSASocket
            (Address.IPv6 () ? AF_INET6 : AF_INET, SOCK_STREAM, IPPROTO_TCP,
                    0, 0, WSA_FLAG_OVERLAPPED)) == INVALID_HANDLE_VALUE)
    {
        Lacewing::Error Error;
       
        Error.Add(WSAGetLastError ());        
        Error.Add("Error creating socket");
        
        if (internal->Handlers.Error)
            internal->Handlers.Error (*this, Error);

        return;
    }

    lwp_disable_ipv6_only ((lwp_socket) internal->Socket);

    internal->Watch = internal->Pump.Add
        (internal->Socket, internal, ::Completion);

    /* LPFN_CONNECTEX and WSAID_CONNECTEX aren't defined w/ MinGW */

    static BOOL (PASCAL FAR * lw_ConnectEx)
    (   
        SOCKET s,
        const struct sockaddr FAR *name,
        size_t namelen,
        PVOID lpSendBuffer,
        DWORD dwSendDataLength,
        LPDWORD lpdwBytesSent,
        LPOVERLAPPED lpOverlapped
    );

    GUID ID = {0x25a207b9,0xddf3,0x4660,
                        {0x8e,0xe9,0x76,0xe5,0x8c,0x74,0x06,0x3e}};

    DWORD bytes = 0;

    WSAIoctl ((SOCKET) internal->Socket, SIO_GET_EXTENSION_FUNCTION_POINTER,
                    &ID, sizeof (ID), &lw_ConnectEx, sizeof (lw_ConnectEx),
                        &bytes, 0, 0);

    assert (lw_ConnectEx);

    sockaddr_storage LocalAddress;
        memset (&LocalAddress, 0, sizeof (LocalAddress));

    if (Address.IPv6 ())
    {
        ((sockaddr_in6 *) &LocalAddress)->sin6_family = AF_INET6;
        ((sockaddr_in6 *) &LocalAddress)->sin6_addr = in6addr_any;       
    }
    else
    {
        ((sockaddr_in *) &LocalAddress)->sin_family = AF_INET;
        ((sockaddr_in *) &LocalAddress)->sin_addr.S_un.S_addr = INADDR_ANY;
    }

    if (bind ((SOCKET) internal->Socket,
            (sockaddr *) &LocalAddress, sizeof (LocalAddress)) == -1)
    {
        Lacewing::Error Error;
       
        Error.Add (WSAGetLastError ());        
        Error.Add ("Error binding socket");
        
        if (internal->Handlers.Error)
            internal->Handlers.Error (*this, Error);

        return;
    }

    delete internal->Address;
    internal->Address = new Lacewing::Address (Address);

    OVERLAPPED * overlapped = new OVERLAPPED;
    memset (overlapped, 0, sizeof (OVERLAPPED));

    if (!lw_ConnectEx ((SOCKET) internal->Socket, Address.internal->Info->ai_addr,
                        Address.internal->Info->ai_addrlen, 0, 0, 0, overlapped))
    {
        int Code = WSAGetLastError ();

        assert(Code == WSA_IO_PENDING);
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

