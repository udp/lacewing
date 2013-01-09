
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
#include "Address.h"

struct Filter::Internal
{
    Internal ()
    {
        LocalPort = RemotePort = 0;
        Local     = Remote     = 0;

        Reuse  = true;
        IPv6   = true;
    }

    ~ Internal ()
    {
        delete Local;
        delete Remote;
    }

    bool Reuse, IPv6;

    Lacewing::Address * Local, * Remote;
    int LocalPort, RemotePort;
};

Filter::Filter ()
{
    internal  = new Filter::Internal;
    Tag       = 0;
}

Filter::Filter (Filter &_Filter)
{
    internal  = new Filter::Internal;
    Tag       = 0;

    IPv6 (_Filter.IPv6 ());
    Reuse (_Filter.Reuse ());

    LocalPort (_Filter.LocalPort ());
    RemotePort (_Filter.RemotePort ());

    Local (_Filter.Local ());
    Remote (_Filter.Remote ());
}

Filter::~Filter ()
{
    delete internal;
}

void Filter::Remote (Lacewing::Address * Address)
{
    if (Address)
    {
        if (Address->Resolve ())
            return;

        delete internal->Remote;
        internal->Remote = new Lacewing::Address (*Address);

        if (internal->RemotePort)
            internal->Remote->Port (internal->RemotePort);
    }
    else
    {
        if (internal->Remote)
            internal->RemotePort = internal->Remote->Port ();
        
        delete internal->Remote;
        internal->Remote = 0;
    }
}

Lacewing::Address * Filter::Remote ()
{
    return internal->Remote;
}

void Filter::Local (Lacewing::Address * Address)
{
    if (Address)
    {
        if (Address->Resolve ())
            return;
    
        delete internal->Local;
        internal->Local = new Lacewing::Address (*Address);

        if (internal->LocalPort)
            internal->Local->Port (internal->LocalPort);
    }
    else
    {
        if (internal->Local)
            internal->LocalPort = internal->Local->Port ();

        delete internal->Local;
        internal->Local = 0;
    }
}

Lacewing::Address * Filter::Local ()
{
    return internal->Local;
}

void Filter::Reuse (bool Enabled)
{
    internal->Reuse = Enabled;
}

bool Filter::Reuse ()
{
    return internal->Reuse;
}

void Filter::IPv6 (bool Enabled)
{
    internal->IPv6 = Enabled;
}

bool Filter::IPv6 ()
{
    return internal->IPv6;
}

int Filter::LocalPort ()
{
    return internal->Local
        ? internal->Local->Port () : internal->LocalPort;
}

void Filter::LocalPort (int Port)
{
    if (internal->Local)
        internal->Local->Port (Port);
    else
        internal->LocalPort = Port;
}

int Filter::RemotePort ()
{
    return internal->Remote
        ? internal->Remote->Port () : internal->RemotePort;
}

void Filter::RemotePort (int Port)
{
    if (internal->Remote)
        internal->Remote->Port (Port);
    else
        internal->RemotePort = Port;
}

/* Used internally by the library to create a server socket from a Filter */

lwp_socket lwp_create_server_socket (Lacewing::Filter &Filter, int Type, int Protocol,
                              Lacewing::Error &Error)
{
    if ((!Filter.IPv6 ()) && ( (Filter.Local () && Filter.Local ()->IPv6 ())
                               || (Filter.Remote () && Filter.Remote ()->IPv6 ()) ))
    {
        Error.Add ("Filter has IPv6 disabled, but one of the attached addresses is an IPv6 address.  "
                    "Try using HINT_IPv4 when constructing the Address object.");

        return -1;
    }

    lwp_socket Socket;
    bool IPv6 = Filter.IPv6 ();

    #ifdef _WIN32

        if (IPv6)
        {
            if ((Socket = WSASocket
                (AF_INET6, Type, Protocol, 0, 0, WSA_FLAG_OVERLAPPED)) == -1)
            {
                if (WSAGetLastError () != WSAEAFNOSUPPORT)
                {
                    Error.Add (WSAGetLastError ());
                    Error.Add ("Error creating socket");

                    return -1;
                }

                IPv6 = false;
            }
        }

        if (!IPv6)
        {
            if ((Socket = WSASocket
                (AF_INET, Type, Protocol, 0, 0, WSA_FLAG_OVERLAPPED)) == -1)
            {
                Error.Add (WSAGetLastError ());
                Error.Add ("Error creating socket");

                return -1;
            }
        }

    #else

        if (IPv6)
        {
            if ((Socket = socket (AF_INET6, Type, Protocol)) == -1)
            {
                if (errno != EAFNOSUPPORT)
                {
                    Error.Add (errno);
                    Error.Add ("Error creating socket");

                    return -1;
                }

                IPv6 = false;
            }
        }

        if (!IPv6)
        {
            if ((Socket = socket (AF_INET, Type, Protocol)) == -1)
            {
                Error.Add (errno);
                Error.Add ("Error creating socket");

                return -1;
            }
        }

        fcntl (Socket, F_SETFL, fcntl (Socket, F_GETFL, 0) | O_NONBLOCK);

    #endif

    if (IPv6)
        lwp_disable_ipv6_only (Socket);

    {   int reuse = Filter.Reuse () ? 1 : 0;
        setsockopt (Socket, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse, sizeof (reuse));
    }

    sockaddr_storage addr;
    memset (&addr, 0, sizeof (sockaddr_storage));

    size_t addr_len = 0;

    if (Filter.Local ())
    {
        addrinfo * info = Filter.Local ()->internal->Info;

        if (info)
        {
            memcpy (&addr, info->ai_addr, info->ai_addrlen);
            addr_len = info->ai_addrlen;
        }
    }

    if (!addr_len)
    {
        if (IPv6)
        {
            addr_len = sizeof (sockaddr_in6);

            ((sockaddr_in6 *) &addr)->sin6_family
                = AF_INET6;

            ((sockaddr_in6 *) &addr)->sin6_addr
                = in6addr_any;

            ((sockaddr_in6 *) &addr)->sin6_port
                = Filter.LocalPort () ? htons (Filter.LocalPort ()) : 0;
        }
        else
        {
            addr_len = sizeof (sockaddr_in);

            ((sockaddr_in *) &addr)->sin_family
                = AF_INET;

            ((sockaddr_in *) &addr)->sin_addr.s_addr
                = INADDR_ANY;

            ((sockaddr_in *) &addr)->sin_port
                = Filter.LocalPort () ? htons (Filter.LocalPort ()) : 0;
        }
    }

    if (bind (Socket, (sockaddr *) &addr, (int) addr_len) == -1)
    {
        Error.Add (lwp_last_socket_error);
        Error.Add ("Error binding socket");

        lwp_close_socket (Socket);

        return -1;
    }

    return Socket;
}

