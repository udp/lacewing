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

#include "Common.h"

const char * const SessionCookie = "lw_session";

void Webserver::Request::Session (const char * key, const char * value)
{
    const char * cookie = Cookie (SessionCookie);

    Webserver::Internal::Session * session;

    if (*cookie)
    {
        HASH_FIND_STR (internal->Server.Sessions, cookie, session);
    }
    else
    {   session = 0;
    }

    if (!session)
    {
        char session_id [SESSION_ID_SIZE];
 
        lw_random (session_id, sizeof (session_id));
 
    	session = new Webserver::Internal::Session;
		memset (session, 0, sizeof (session));

        for (int i = 0; i < sizeof (session_id); ++ i)
            sprintf (session->session_key + i * 2, "%02X", (unsigned char) session_id [i]);
 
        HASH_ADD_STR (internal->Server.Sessions, session_key, session);

	Cookie(SessionCookie, session->session_key);
    }

    lw_nvhash_set (&session->data, key, value, lw_true);
}

const char * Webserver::Request::Session (const char * key)
{
    const char * cookie = Cookie (SessionCookie);

    if (!*cookie)
        return "";

    Webserver::Internal::Session * session;
    HASH_FIND_STR (internal->Server.Sessions, cookie, session);

    if (!session)
        return "";

    return lw_nvhash_get (&session->data, key, "");
}

void Webserver::CloseSession (const char * id)
{
    Webserver::Internal::Session * session;
    HASH_FIND_STR (internal->Sessions, id, session);

    if (!session)
        return;

    lw_nvhash_clear (&session->data);
    HASH_DEL (internal->Sessions, session);
}

void Webserver::Request::CloseSession ()
{
    internal->Server.Webserver.CloseSession (Session ());
}

const char * Webserver::Request::Session ()
{
    return Cookie (SessionCookie);
}

Webserver::Request::SessionItem * Webserver::Request::FirstSessionItem ()
{
    const char * cookie = Cookie (SessionCookie);

    if (!*cookie)
        return 0;

    Webserver::Internal::Session * session;
    HASH_FIND_STR (internal->Server.Sessions, cookie, session);

    if (!session)
        return 0;

    return (SessionItem *) session->data;
}

Webserver::Request::SessionItem *
        Webserver::Request::SessionItem::Next ()
{
    return (SessionItem *) ((lw_nvhash *) this)->hh.next;
}

const char * Webserver::Request::SessionItem::Name ()
{
    return ((lw_nvhash *) this)->key;
}

const char * Webserver::Request::SessionItem::Value ()
{
    return ((lw_nvhash *) this)->value;
}

