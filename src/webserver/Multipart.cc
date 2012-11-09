
/* vim: set et ts=4 sw=4 ft=cpp:
 *
 * Copyright (C) 2012 James McLaughlin et al.  All rights reserved.
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

int Multipart::onHeaderField (const char * at, size_t length)
{
    CurHeaderName = at;
    CurHeaderNameLength = length;

    return 0;
}

int Multipart::onHeaderValue (const char * at, size_t length)
{
    WebserverHeader header; 

    if (! (header.Name = (char *) malloc (CurHeaderNameLength + 1)))
        return -1;

    if (! (header.Value = (char *) malloc (length + 1)))
        return -1;

    memcpy (header.Name, CurHeaderName, CurHeaderNameLength);
    header.Name [CurHeaderNameLength] = 0;

    memcpy (header.Value, at, length);
    header.Value [length] = 0;

    lwp_trace ("Multipart %p: Got header: %s  =>  %s", this, header.Name, header.Value);

    Headers.Push (header);

    if (!strcasecmp (header.Name, "Content-Disposition"))
    {
        if (!ParseDisposition (length, at))
            return -1;

        if (Child)
        {
            lw_nvhash_set (&Child->Disposition, "name",
                            lw_nvhash_get (&Disposition, "name", ""), lw_true);
        }
    }
    else if (!strcasecmp (header.Name, "Content-Type"))
    {
        if (lwp_begins_with (header.Value, "multipart"))
        {
            Child = new Multipart (Server, Request, header.Value);

            const char * name = lw_nvhash_get (&Disposition, "name", 0);

            if (name)
                lw_nvhash_set (&Child->Disposition, "name", name, lw_true);
        }
    }

    return 0;
}

bool Multipart::ParseDisposition (size_t length, const char * disposition)
{
    lw_nvhash_clear (&Disposition);

    const char s_type = 0,
               s_param_name = 1,
               s_param_value_start = 2,
               s_param_value = 3,
               s_param_value_end = 4;

    char state = s_type;

    size_t name_begin = 0, value_begin = 0, name_len;

    for (size_t i = 0; i < length; ++ i)
    {
        char c = disposition [i];

        switch (state)
        {
        case s_type:

            if (c == ';' || !c)
            {
                lw_nvhash_set_ex
                    (&Disposition, 4, "type", i, disposition, lw_true);

                if (!c)
                    return true;

                state = s_param_name;

                continue;
            }

            continue;

        case s_param_value_end:

            if (!c)
                return true;

            state = s_param_name;

            if (c == ';')
                continue;
            else
                return false;

        /* fallthrough */
        case s_param_name:

            if (!c)
                return true;

            if (!name_begin)
            {
                if (isspace (c))
                    continue;

                name_begin = i;
            }

            if (c == '=')
            {
                name_len = i - name_begin;
                state = s_param_value_start;

                continue;
            }

            continue;

        case s_param_value_start:

            state = s_param_value;

            if (c == '"')
                continue;

        /* fallthrough */
        case s_param_value:

            if (!value_begin)
                value_begin = i;

            if (c == '"' || c == ';' || !c)
            {
                lw_nvhash_set_ex
                    (&Disposition, name_len, disposition + name_begin,
                        i - value_begin, disposition + value_begin, lw_true);

                if (!c)
                    return true;

                name_begin = 0;
                value_begin = 0;

                state = (c == '"') ? s_param_value_end : s_param_name;

                continue;
            }

            continue;

        };
    }

    return true;
}

int Multipart::onHeadersComplete ()
{
    lwp_trace ("Multipart %p: onHeadersComplete", this);

    ParsingHeaders = false;
    Request.Buffer.Reset ();

    if (lw_nvhash_get (&Disposition, "filename", 0))
    {
        /* A filename was given - assign this part an Upload structure. */
        
        CurUpload = new Webserver::Upload::Internal (Request);

        CurUpload->Disposition = Disposition;
        Disposition = 0;

        for (List <WebserverHeader>::Element * E
                = Headers.First; E; E = E->Next)
        {
            CurUpload->Headers.Push (** E);
        }

        Headers.Clear ();

        lwp_trace ("Multipart %p: Calling UploadStart handler", this);

        if (Server.Handlers.UploadStart)
        {
            Server.Handlers.UploadStart
                (Server.Webserver, Request, *CurUpload);
        }
    }

    return 0;
}

int Multipart::onPartData (const char * at, size_t length)
{
    if (Child)
    {
        if (Child->Process (at, length) != length)
        {
            delete Child;
            Child = 0;

            return -1;
        }

        if (Child->Done)
        {
            delete Child;
            Child = 0;

            return 0;
        }
    }

    if (!CurUpload)
    {
        /* No Upload structure: this will be treated as a normal POST item, and
         * so the data must be buffered.
         */

        Request.Buffer.Add (at, length);
        return 0;
    }

    if (CurUpload->AutoSaveFile)
    {
        /* Auto save mode */

        CurUpload->AutoSaveFile->Write (at, length);
        return 0;
    }

    /* Manual save mode */

    if (Server.Handlers.UploadChunk)
    {
        Server.Handlers.UploadChunk
            (Server.Webserver, Request, *CurUpload, at, length);
    }

    return 0;
}

int Multipart::onPartDataBegin ()
{
    lwp_trace ("Multipart %p: onPartDataBegin", this);

    return 0;
}

int Multipart::onPartDataEnd ()
{
    lwp_trace ("Multipart %p: onPartDataEnd", this);

    ParsingHeaders = true;

    if (CurUpload)
    {
        if (Parent)
            Parent->AddUpload ((Webserver::Upload *) CurUpload);
        else
            AddUpload ((Webserver::Upload *) CurUpload);

        if (CurUpload->AutoSaveFile)
        {
            /* Auto save */

            lwp_trace("Closing auto save file");

            CurUpload->AutoSaveFile->Close (false);
        }
        else
        {
            /* Manual save */

            if (Server.Handlers.UploadDone)
            {
                Server.Handlers.UploadDone
                    (Server.Webserver, Request, *CurUpload);
            }
        }

        CurUpload = 0;
    }
    else
    {
        /* No Upload structure - add to PostItems */

        Request.Buffer.Add <char> (0);

        lw_nvhash_set (&Request.PostItems,
                       lw_nvhash_get (&Disposition, "name", ""),
                       Request.Buffer.Buffer,
                       lw_true);

        Request.Buffer.Reset ();
    }

    lw_nvhash_clear (&Disposition);

    return 0;
}

int Multipart::onBodyEnd ()
{
    lwp_trace ("Multipart %p: onBodyEnd", this);

    Done = true;

    if (!Parent)
        TryCallHandler ();

    return 0;
}

void Multipart::TryCallHandler ()
{
    /* Only call the handler if all files are closed */

    for (int i = 0; i < NumUploads; ++ i)
    {
        if (Uploads [i]->internal->AutoSaveFile)
            return;
    }

    Request.BeforeHandler ();

    if (Server.Handlers.UploadPost)
    {
        Server.Handlers.UploadPost
           (Server.Webserver, Request, Uploads, NumUploads);
    }

    Request.AfterHandler ();
}
    
static int onHeaderField (multipart_parser * parser, const char * at, size_t length)
{   return ((Multipart *) multipart_parser_get_data (parser))->onHeaderField (at, length);
}
static int onHeaderValue (multipart_parser * parser, const char * at, size_t length)
{   return ((Multipart *) multipart_parser_get_data (parser))->onHeaderValue (at, length);
}
static int onPartData (multipart_parser * parser, const char * at, size_t length)
{   return ((Multipart *) multipart_parser_get_data (parser))->onPartData (at, length);
}
static int onPartDataBegin (multipart_parser * parser)
{   return ((Multipart *) multipart_parser_get_data (parser))->onPartDataBegin ();
}
static int onHeadersComplete (multipart_parser * parser)
{   return ((Multipart *) multipart_parser_get_data (parser))->onHeadersComplete ();
}
static int onPartDataEnd (multipart_parser * parser)
{   return ((Multipart *) multipart_parser_get_data (parser))->onPartDataEnd ();
}
static int onBodyEnd (multipart_parser * parser)
{   return ((Multipart *) multipart_parser_get_data (parser))->onBodyEnd ();
}

const multipart_parser_settings settings =
{
    ::onHeaderField,
    ::onHeaderValue,
    ::onPartData,

    ::onPartDataBegin,
    ::onHeadersComplete,
    ::onPartDataEnd,
    ::onBodyEnd
};

Multipart::Multipart
    (Webserver::Internal &server, Webserver::Request::Internal &request,
         const char * content_type) : Request (request), Server (server)
{
    Parent = Child = 0;

    Uploads = 0;
    NumUploads = 0;

    CurUpload = 0;

    Done = false;

    char * boundary;

    {   const char * _boundary = strstr (content_type, "boundary=") + 9;

        boundary = (char *) alloca (strlen (_boundary) + 3);

        strcpy (boundary, "--");
        strcat (boundary, _boundary);
    }

    lwp_trace ("Creating parser with boundary: %s", boundary);

    Parser = multipart_parser_init (boundary, &settings);
    multipart_parser_set_data (Parser, this);

    ParsingHeaders = true;

    Disposition = 0;
}

Multipart::~ Multipart ()
{
    multipart_parser_free (Parser);
    
    lw_nvhash_clear (&Disposition);

    while (Headers.Last)
    {
        WebserverHeader &header = (** Headers.Last);

        free (header.Name);
        free (header.Value);

        Headers.Pop ();
    }

    if (Child)
        delete Child;

    free (Uploads);
}

size_t Multipart::Process (const char * buffer, size_t buffer_size)
{
    size_t size = buffer_size;

    assert (size != 0);

    /* TODO : This code is duplicated in HTTP.cc for HTTP headers */

    if (ParsingHeaders)
    {
        for (size_t i = 0; i < size; )
        {
            {   char b = buffer [i];

                if (b == '\r')
                {
                    if (buffer [i + 1] == '\n')
                        ++ i;
                }
                else if (b != '\n')
                {
                    ++ i;
                    continue;
                }
            }

            size_t toParse = i + 1;
            bool error = false;

            if (Request.Buffer.Size)
            {
                Request.Buffer.Add (buffer, toParse);

                size_t parsed = multipart_parser_execute
                    (Parser, Request.Buffer.Buffer, Request.Buffer.Size);

                if (parsed != Request.Buffer.Size)
                    error = true;

                Request.Buffer.Reset ();
            }
            else
            {
                size_t parsed = multipart_parser_execute
                    (Parser, buffer, toParse);

                if (parsed != toParse)
                    error = true;
            }

            size -= toParse;
            buffer += toParse;

            if (error)
            {
                lwp_trace ("Multipart error");
                return 0;
            }

            if (!ParsingHeaders)
                break;

            i = 0;
        }

        if (ParsingHeaders)
        {
            /* TODO : max line length */

            Request.Buffer.Add (buffer, size);
            return buffer_size;
        }
        else
        {
            if (!size)
                return buffer_size;
        }
    }

    size_t parsed = multipart_parser_execute (Parser, buffer, size);

    if (parsed != size)
        return 0;

    return buffer_size;
}



