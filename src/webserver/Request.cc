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

Webserver::Request::Request (Pump &_Pump) : Stream (_Pump)
{
}

Webserver::Request::~ Request ()
{
}

Webserver::Request::Internal::Internal
    (Webserver::Internal &_Server, WebserverClient &_Client)
        : Server (_Server), Client (_Client), Webserver::Request (_Server.Pump)
{
    Tag = 0;
    internal = this;

    Cookies = 0;
    GetItems = 0;
    PostItems = 0;

    Responded = true;
}

Webserver::Request::Internal::~ Internal ()
{
}

void Webserver::Request::Internal::Clean ()
{
    Responded = true;
    ParsedPostData = false;

    Version_Major = 0;
    Version_Minor = 0;

    while (InHeaders.Last)
    {
        WebserverHeader &header = (** InHeaders.Last);

        free (header.Name);
        free (header.Value);

        InHeaders.Pop ();
    }

    while (OutHeaders.Last)
    {
        WebserverHeader &header = (** OutHeaders.Last);

        free (header.Name);
        free (header.Value);

        OutHeaders.Pop ();
    }

    if (Cookies)
    {
        Cookie * cookie, * tmp;

        HASH_ITER (hh, Cookies, cookie, tmp)
        {
            HASH_DEL (Cookies, cookie);

            free (cookie->Name);
            free (cookie->Value);

            free (cookie);
        }
    }
    
    lw_nvhash_clear (&GetItems);
    lw_nvhash_clear (&PostItems);

    Cookies    = 0;
    GetItems   = 0;
    PostItems  = 0;

    *Method   = 0;
    *URL      = 0;
    *Hostname = 0;
}

void Webserver::Request::Internal::BeforeHandler ()
{
    /* Any preparation to be done immediately before calling the handler should
     * be in this function.
     */

    Buffer.Add <char> (0); /* null terminate body */

    strcpy(Status, "200 OK");

    OutHeaders.Clear();
    
    AddHeader ("Server", lw_version ());
    AddHeader ("Content-Type", "text/html; charset=UTF-8");

    if(Client.Secure)
    {
        /* When the request is secure, add the "Cache-Control: public" header
         * by default.  DisableCache() called in the handler will remove this,
         * and should be used for any pages containing sensitive data that
         * shouldn't be cached.
         */

        AddHeader ("Cache-Control", "public");
    }   

    assert (Responded);

    Responded = false;
}

void Webserver::Request::Internal::AfterHandler ()
{
    /* Anything to be done immediately after the handler has returned should be
     * in this function.
     */
    
    if((!Responded) && Server.AutoFinish)
        Respond ();
}

void Webserver::Request::Internal::RunStandardHandler ()
{
    /* If the protocol doesn't want to call the handler itself (i.e. it's a
     * standard GET/POST/HEAD request), it will call this function to invoke
     * the appropriate handler automatically.
     */

    BeforeHandler ();

    do
    {
        if(!strcmp(Method, "GET"))
        {
            if (Server.Handlers.Get)
                Server.Handlers.Get (Server.Webserver, *this);

            break;
        }

        if(!strcmp(Method, "POST"))
        {
            if (Server.Handlers.Post)
                Server.Handlers.Post (Server.Webserver, *this);

            break;
        }

        if(!strcmp(Method, "HEAD"))
        {
            if (Server.Handlers.Head)
                Server.Handlers.Head (Server.Webserver, *this);

            break;
        }

        ((Request *) this)->Status (501, "Not Implemented");

        Finish ();

        return;

    } while(0);

    AfterHandler ();
}

bool Webserver::Request::Internal::In_Version (size_t len, const char * version)
{
    if (len != 8)
        return false;

    if (memcmp (version, "HTTP/", 5) || version [6] != '.')
        return false;

    Version_Major = version [5] - '0';
    Version_Minor = version [7] - '0';

    if (Version_Major != 1)
        return false;

    if (Version_Minor != 0 && Version_Minor != 1)
        return false;

    return true;
}

bool Webserver::Request::Internal::In_Method (size_t len, const char * method)
{
    if (len > (sizeof (Method) - 1))
        return false;

    memcpy (Method, method, len);
    Method [len] = 0;

    return true;
}

static lw_bool parse_cookie_header (Webserver::Request::Internal * internal,
                                    size_t header_len, const char * header)
{
    size_t name_begin, name_len, value_begin, value_len;

    int state = 0;

    for (size_t i = 0 ;; ++ i)
    {
        switch (state)
        {
        case 0: /* seeking name */

            if (i >= header_len)
                return lw_true;

            if (header [i] == ' ' || header [i] == '\t')
                break;

            name_begin = i;
            name_len = 1;

            ++ state;

            break;

        case 1: /* seeking name/value separator */

            if (i >= header_len)
                return lw_false;

            if (header [i] == '=')
            {
                value_begin = i + 1;
                value_len = 0;

                ++ state;
                break;
            }

            ++ name_len;
            break;

        case 2: /* seeking end of value */

            if (i >= header_len || header [i] == ';')
            {
                internal->SetCookie
                    (name_len, header + name_begin,
                        value_len, header + value_begin, 0, "", false);
        
                if (i >= header_len)
                    return lw_true;

                state = 0;
                break;
            }

            ++ value_len;
        };
    }
}

bool Webserver::Request::Internal::In_Header
    (size_t name_len, const char * name, size_t value_len, const char * value)
{
    {   WebserverHeader header;

        /* TODO : limit name_len/value_len */

        if (! (header.Name = (char *) malloc (name_len + 1)))
            return lw_false;

        if (! (header.Value = (char *) malloc (value_len + 1)))
            return lw_false;
        
        for (size_t i = 0; i < name_len; ++ i)
            header.Name [i] = tolower (name [i]);

        header.Name [name_len] = 0;

        name = header.Name;

        memcpy (header.Value, value, value_len);
        header.Value [value_len] = 0;

        InHeaders.Push (header);
    }

    if (!strcmp (name, "cookie"))
        return parse_cookie_header (internal, value_len, value);

    if (!strcmp (name, "host"))
    {
        /* The hostname gets stored separately with the port removed for
         * the Request.Hostname() function.
         */

        if (value_len > (sizeof (this->Hostname) - 1))
            return false; /* hostname too long */

        size_t host_len = value_len;

        for (size_t i = 0; i < value_len; ++ i)
        {
            if(i > 0 && value [i] == ':')
            {
                host_len = (i - 1);
                break;
            }
        }

        memcpy (this->Hostname, value, host_len);
        this->Hostname [host_len] = 0;

        return lw_true;
    }   
    
    return lw_true;
}

static inline bool GetField (const char * URL, http_parser_url &parsed,
                             int field, const char * &buf, size_t &length)
{
    if (! (parsed.field_set & (1 << field)))
        return false;

    buf = URL + parsed.field_data [field].off;
    length = parsed.field_data [field].len;

    return true;
}

bool Webserver::Request::Internal::In_URL (size_t length, const char * URL)
{
    /* Check for any directory traversal in the URL. */

    for (size_t i = 0; i < (length - 1); ++ i)
    {
        if(URL [i] == '.' && URL [i + 1] == '.')
            return false;
    }

    /* Now hand it over to the URL parser */

    http_parser_url parsed;

    if (http_parser_parse_url (URL, length, 0, &parsed))
        return false;


    /* Path */

    *this->URL = 0;

    {   const char * path;
        size_t path_length;

        if (GetField (URL, parsed, UF_PATH, path, path_length))
        {
            if (*path == '/')
            {
                ++ path;
                -- path_length;
            }

            if (!lwp_urldecode
                  (path, path_length, this->URL, sizeof (this->URL), lw_false))
            {
                return false;
            }
        }
    }


    /* Host */

    {   const char * host;
        size_t host_length;

        if (GetField (URL, parsed, UF_HOST, host, host_length))
        {
            if (parsed.field_set & (1 << UF_PORT))
            {
                /* TODO : Is this robust? */

                host_length += 1 + parsed.field_data [UF_PORT].off;
            }

            In_Header (4, "Host", host_length, host);
        }
    }


    /* GET data */

    {   const char * data;
        size_t length;

        if (GetField (URL, parsed, UF_QUERY, data, length))
        {
            for (;;)
            {
                const char * name = data;

                if (!lwp_find_char (&data, &length, '='))
                    break;

                size_t name_length = data - name;

                /* skip the = character */

                ++ data;
                -- length;

                const char * value = data;
                size_t value_length = length;

                if (lwp_find_char (&data, &length, '&'))
                {
                    value_length = data - value;

                    /* skip the & character */

                    ++ data;
                    -- length;
                }

                char * name_decoded = (char *) malloc (name_length + 1),
                     * value_decoded = (char *) malloc (value_length + 1);

                if(!lwp_urldecode (name, name_length, name_decoded, name_length + 1, lw_true)
                    || !lwp_urldecode (value, value_length, value_decoded, value_length + 1, lw_true))
                {
                    free (name_decoded);
                    free (value_decoded);
                }
                else
                {
                    lw_nvhash_set (&GetItems, name_decoded, value_decoded, lw_false);
                }
            }
        }
    }

    return true;
}

void Webserver::Request::Internal::Respond ()
{
    assert (!Responded);

    /* Respond may delete us w/ SPDY blah blah */

    Client.Respond (*this);
}

Address &Webserver::Request::GetAddress ()
{
    return internal->Client.Socket.GetAddress ();
}

void Webserver::Request::Disconnect ()
{
    internal->Client.Socket.Close ();
}

void Webserver::Request::GuessMimeType (const char * filename)
{
    SetMimeType (lw_guess_mime_type (filename));
}

void Webserver::Request::SetMimeType (const char * MimeType, const char * Charset)
{
     if (!*Charset)
     {
         Header ("Content-Type", MimeType);
         return;
     }

     char Type [256];

     lwp_snprintf (Type, sizeof (Type), "%s; charset=%s", MimeType, Charset);
     Header ("Content-Type", Type);
}

void Webserver::Request::SetRedirect (const char * URL)
{
    Status (303, "See Other");
    Header ("Location", URL);
}

void Webserver::Request::DisableCache ()
{
    Header("Cache-Control", "no-cache");
}

void Webserver::Request::Header (const char * name, const char * value)
{
    for (List <WebserverHeader>::Element * E
            = internal->OutHeaders.First; E; E = E->Next)
    {
        WebserverHeader &header = (** E);

        if (!strcasecmp (header.Name, name))
        {
            free (header.Value);
            header.Value = strdup (value);

            return;
        }
    }

    AddHeader (name, value);
}

void Webserver::Request::AddHeader (const char * name, const char * value)
{
    size_t name_len = strlen (name);

    WebserverHeader header;

    header.Name = (char *) malloc (name_len + 1);

    header.Name [name_len] = 0;

    for (size_t i = 0; i < name_len; ++ i)
        header.Name [i] = tolower (name [i]);

    header.Value = strdup (value);

    internal->OutHeaders.Push (header);
}

void Webserver::Request::Cookie (const char * Name, const char * Value)
{
    Cookie (Name, Value, Secure () ? "Secure; HttpOnly" : "HttpOnly");
}

void Webserver::Request::Cookie
        (const char * name, const char * value, const char * attr)
{
    internal->SetCookie (strlen (name), name,
                         strlen (value), value,
                         strlen (attr), attr, true);
}

void Webserver::Request::Internal::SetCookie
        (size_t name_len, const char * name,
         size_t value_len, const char * value,
         size_t attr_len, const char * attr, bool changed)
{
    Request::Internal::Cookie * cookie;

    HASH_FIND (hh, Cookies, name, name_len, cookie);

    if (!cookie)
    {
        cookie = (Request::Internal::Cookie *) malloc (sizeof (*cookie));
        memset (cookie, 0, sizeof (*cookie));

        cookie->Changed = changed;

        cookie->Name = (char *) malloc (name_len + 1);
        memcpy (cookie->Name, name, name_len);
        cookie->Name [name_len] = 0;

        cookie->Value = (char *) malloc (value_len + 1);
        memcpy (cookie->Value, value, value_len);
        cookie->Value [value_len] = 0;

        cookie->Attr = (char *) malloc (attr_len + 1);
        memcpy (cookie->Attr, attr, attr_len);
        cookie->Attr [attr_len] = 0;

        HASH_ADD_KEYPTR (hh, Cookies, cookie->Name, name_len, cookie);

        return;
    }
    
    cookie->Changed = changed;

    cookie->Value = (char *) realloc (cookie->Value, value_len + 1);
    memcpy (cookie->Value, value, value_len);
    cookie->Value [value_len] = 0;

    cookie->Attr = (char *) realloc (cookie->Attr, attr_len + 1);
    memcpy (cookie->Attr, attr, attr_len);
    cookie->Attr [attr_len] = 0;
}

void Webserver::Request::Status (int code, const char * message)
{
    /* TODO : prevent buffer overrun */

    sprintf (internal->Status, "%d %s", code, message);
}

void Webserver::Request::SetUnmodified ()
{
    Status (304, "Not Modified");
}

void Webserver::Request::LastModified (lw_i64 _time)
{
    struct tm tm;
    time_t time;

    time = (time_t) _time;

    #ifdef _WIN32
        memcpy (&tm, gmtime (&time), sizeof (struct tm));
    #else
        gmtime_r (&time, &tm);
    #endif

    char LastModified [128];
    sprintf (LastModified, "%s, %02d %s %d %02d:%02d:%02d GMT", lwp_weekdays [tm.tm_wday], tm.tm_mday,
                            lwp_months [tm.tm_mon], tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);

    Header("Last-Modified", LastModified);
}

void Webserver::Request::Finish ()
{
    internal->Respond ();
}

const char * Webserver::Request::Header (const char * name)
{
    for (List <WebserverHeader>::Element * E
            = internal->InHeaders.First; E; E = E->Next)
    {
        if (!strcasecmp ((** E).Name, name))
            return (** E).Value;
    }

    return "";
}

struct Webserver::Request::Header * Webserver::Request::FirstHeader ()
{
    return (struct Webserver::Request::Header *) internal->InHeaders.First;
}

const char * Webserver::Request::Header::Name ()
{
    return (** (List <WebserverHeader>::Element *) this).Name;
}

const char * Webserver::Request::Header::Value ()
{
    return (** (List <WebserverHeader>::Element *) this).Value;
}

struct Webserver::Request::Header * Webserver::Request::Header::Next ()
{
    return (struct Webserver::Request::Header *)
              ((List <WebserverHeader>::Element *) this)->Next;
}

const char * Webserver::Request::Cookie (const char * name)
{
    Request::Internal::Cookie * cookie;

    HASH_FIND_STR (internal->Cookies, name, cookie);

    return cookie ? cookie->Value : "";
}

struct Webserver::Request::Cookie * Webserver::Request::FirstCookie ()
{
    return (struct Webserver::Request::Cookie *) internal->Cookies;
}

struct Webserver::Request::Cookie * Webserver::Request::Cookie::Next ()
{
    return (struct Webserver::Request::Cookie *)
                ((Request::Internal::Cookie *) this)->hh.next;
}

const char * Webserver::Request::Cookie::Name ()
{
    return ((Request::Internal::Cookie *) this)->Name;
}

const char * Webserver::Request::Cookie::Value ()
{
    return ((Request::Internal::Cookie *) this)->Value;
}

const char * Webserver::Request::Body ()
{
    return internal->Buffer.Buffer;
}

const char * Webserver::Request::GET (const char * name)
{
    return lw_nvhash_get (&internal->GetItems, name, "");
}

void Webserver::Request::Internal::ParsePostData ()
{
    if (ParsedPostData)
        return;

    ParsedPostData = true;

    if (!lwp_begins_with
            (Header ("content-type"), "application/x-www-form-urlencoded"))
    {
        return;
    }

    char * post_data = Buffer.Buffer,
             * end = post_data + Buffer.Size, b = *end;

    *end = 0;

    for (;;)
    {
        char * name = post_data, * value = strchr (post_data, '=');

        size_t name_length = value - name;

        if (!value ++)
            break;

        char * next = strchr (value, '&');

        size_t value_length = next ? next - value : strlen (value);

        char * name_decoded = (char *) malloc (name_length + 1),
             * value_decoded = (char *) malloc (value_length + 1);

        if(!lwp_urldecode (name, name_length, name_decoded, name_length, lw_true)
                || !lwp_urldecode (value, value_length, value_decoded, value_length, lw_true))
        {
            free (name_decoded);
            free (value_decoded);
        }
        else
        {
            lw_nvhash_set (&PostItems, name_decoded, value_decoded, lw_false);
        }

        if (!next)
            break;

        post_data = next + 1;
    }

    *end = b;
}

const char * Webserver::Request::POST (const char * name)
{
    internal->ParsePostData ();

    return lw_nvhash_get (&internal->PostItems, name, "");
}

Webserver::Request::Parameter * Webserver::Request::GET ()
{
    return (Webserver::Request::Parameter *) internal->GetItems;
}

Webserver::Request::Parameter * Webserver::Request::POST ()
{
    internal->ParsePostData ();

    return (Webserver::Request::Parameter *) internal->PostItems;
}

Webserver::Request::Parameter *
        Webserver::Request::Parameter::Next ()
{
    return (Webserver::Request::Parameter *) ((lw_nvhash *) this)->hh.next;
}

const char * Webserver::Request::Parameter::Name ()
{
    return ((lw_nvhash *) this)->key;
}

const char * Webserver::Request::Parameter::Value ()
{
    return ((lw_nvhash *) this)->value;
}

lw_i64 Webserver::Request::LastModified ()
{
    const char * LastModified = Header("If-Modified-Since");

    if(*LastModified)
        return lwp_parse_time (LastModified);

    return 0;
}

bool Webserver::Request::Secure ()
{
    return internal->Client.Secure;
}

const char * Webserver::Request::Hostname ()
{
    return internal->Hostname;
}

const char * Webserver::Request::URL ()
{
    return internal->URL;
}

int Webserver::Request::IdleTimeout ()
{
    return internal->Client.Timeout;
}

void Webserver::Request::IdleTimeout (int seconds)
{
    internal->Client.Timeout = seconds;
}

const char * Webserver::Upload::Filename ()
{
    const char * filename = lw_nvhash_get
        (&internal->Disposition, "filename", ""), * slash;

    /* Old versions of IE send the absolute path */

    if ((slash = strrchr (filename, '/')))
        return slash + 1;

    if ((slash = strrchr (filename, '\\')))
        return slash + 1;

    return filename;
}

const char * Webserver::Upload::FormElementName ()
{
    return lw_nvhash_get (&internal->Disposition, "name", "");
}

const char * Webserver::Upload::Header (const char * name)
{
    for (List <WebserverHeader>::Element * E
            = internal->Headers.First; E; E = E->Next)
    {
        if (!strcasecmp ((** E).Name, name))
            return (** E).Value;
    }

    return "";
}

struct Webserver::Upload::Header * Webserver::Upload::FirstHeader ()
{
    return (struct Webserver::Upload::Header *) internal->Headers.First;
}

const char * Webserver::Upload::Header::Name ()
{
    return (** (List <WebserverHeader>::Element *) this).Name;
}

const char * Webserver::Upload::Header::Value ()
{
    return (** (List <WebserverHeader>::Element *) this).Value;
}

struct Webserver::Upload::Header * Webserver::Upload::Header::Next ()
{
    return (struct Webserver::Upload::Header *)
                ((List <WebserverHeader>::Element *) this)->Next;
}

void Webserver::Upload::SetAutoSave ()
{
    internal->SetAutoSave ();
}

static void onAutoSaveFileClose (Lacewing::Stream &stream, void * tag)
{
    Webserver::Upload::Internal * upload = (Webserver::Upload::Internal *) tag;

    delete upload->AutoSaveFile;
    upload->AutoSaveFile = 0;

    upload->Request.Client.Multipart->TryCallHandler ();
}

void Webserver::Upload::Internal::SetAutoSave ()
{
    if (AutoSaveFile)
        return;

    AutoSaveFile = new File (Request.Server.Pump);

    AutoSaveFile->AddHandlerClose (onAutoSaveFileClose, this);
    AutoSaveFile->OpenTemp ();

    free (AutoSaveFilename);
    AutoSaveFilename = strdup (AutoSaveFile->Name ());
}

const char * Webserver::Upload::GetAutoSaveFilename ()
{
    if (!internal->AutoSaveFilename)
        return "";

    return internal->AutoSaveFilename;
}


