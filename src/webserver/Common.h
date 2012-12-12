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
#include "../HeapBuffer.h"
#include "../../deps/multipart-parser/multipart_parser.h"

#define SESSION_ID_SIZE 32

class WebserverClient;

struct WebserverHeader
{
    char * Name, * Value;
    WebserverHeader * Next;
};

struct Webserver::Upload::Internal : public Webserver::Upload
{
    Webserver::Request::Internal &Request;

    lw_nvhash * Disposition;
    
    File * AutoSaveFile;
    char * AutoSaveFilename;

    void SetAutoSave ();

    List <WebserverHeader> Headers;

    inline Internal (Webserver::Request::Internal &_Request)
        : Request (_Request)
    {
        internal = this;
        Tag = 0;

        AutoSaveFile = 0;
        AutoSaveFilename = 0;
    }

    virtual inline ~ Internal ()
    {
        lwp_trace("Free upload!");

        lw_nvhash_clear (&Disposition);

        while (Headers.Last)
        {
            WebserverHeader &header = (** Headers.Last);

            free (header.Name);
            free (header.Value);

            Headers.Pop ();
        }

        delete AutoSaveFile;

        free (AutoSaveFilename);
    }
};

struct Multipart
{
    Webserver::Internal &Server;
    Webserver::Request::Internal &Request;

    Multipart * Parent, * Child;

    multipart_parser * Parser;

    Multipart (Webserver::Internal &, Webserver::Request::Internal &,
               const char * content_type);

    ~ Multipart ();

    size_t Process (const char * buffer, size_t size);
    bool Done;

    lw_nvhash * Disposition;

    /* Call the handler if all auto save files are now closed */

    void TryCallHandler ();

    /* Multipart parser callbacks */

    int onHeaderField (const char * at, size_t length);
    int onHeaderValue (const char * at, size_t length);
    int onPartData (const char * at, size_t length);
    int onPartDataBegin ();
    int onHeadersComplete ();
    int onPartDataEnd ();
    int onBodyEnd ();

protected:

    bool ParsingHeaders;

    const char * CurHeaderName;
    size_t CurHeaderNameLength;
    
    List <WebserverHeader> Headers;

    Webserver::Upload::Internal * CurUpload;

    Webserver::Upload ** Uploads;
    int NumUploads;

    inline void AddUpload (Webserver::Upload * upload)
    {
        Uploads = (Webserver::Upload **)
            realloc (Uploads, sizeof (Webserver::Upload *) * (++ NumUploads));

        Uploads [NumUploads - 1] = upload;
    }

    bool ParseDisposition (size_t length, const char * disposition);
};

struct Webserver::Internal
{
    Lacewing::Pump &Pump;

    Lacewing::Server * Socket, * SecureSocket;
    Lacewing::Timer Timer;

    struct Session
    {
        lw_nvhash * data;
        UT_hash_handle hh;
    	char session_key[SESSION_ID_SIZE * 2 + 1];
    };
    
    Session * Sessions;

    bool AutoFinish;

    static void SocketConnect(Lacewing::Server &, Lacewing::Server::Client &);
    static void SocketDisconnect(Lacewing::Server &, Lacewing::Server::Client &);
    static void SocketReceive(Lacewing::Server &, Lacewing::Server::Client &, char * Buffer, size_t);
    static void SocketError(Lacewing::Server &, Lacewing::Error &);

    inline void PrepareSocket()
    {
        if(!Socket)
        {
            Socket = new Lacewing::Server (Pump);

            Socket->Tag = this;

            Socket->onConnect    (SocketConnect);
            Socket->onDisconnect (SocketDisconnect);
            Socket->onError      (SocketError);
        }

        StartTimer ();
    }

    inline void PrepareSecureSocket()
    {
        if(!SecureSocket)
        {
            SecureSocket = new Lacewing::Server (Pump);
    
            SecureSocket->Tag = this;

            SecureSocket->onConnect    (SocketConnect);
            SecureSocket->onDisconnect (SocketDisconnect);
            SecureSocket->onError      (SocketError);

            #ifndef LacewingNoSPDY
                SecureSocket->AddNPN ("spdy/3");
                SecureSocket->AddNPN ("spdy/2");
            #endif

            SecureSocket->AddNPN ("http/1.1");
            SecureSocket->AddNPN ("http/1.0");
        }
        
        StartTimer ();
    }

    int Timeout;

    inline void StartTimer ()
    {
        #ifdef LacewingTimeoutExperiment

            if (Timer.Started())
                return;

            Timer.Start (Timeout * 1000);
        
        #endif
    }

    inline void StopTimer ()
    {
        Timer.Stop ();
    }

    Lacewing::Webserver &Webserver;

    struct
    {
        HandlerError        Error;
        HandlerGet          Get;
        HandlerPost         Post;
        HandlerHead         Head;
        HandlerUploadStart  UploadStart;
        HandlerUploadChunk  UploadChunk;
        HandlerUploadDone   UploadDone;
        HandlerUploadPost   UploadPost;
        HandlerDisconnect   Disconnect;

    } Handlers;

    inline Internal (Lacewing::Webserver &_Webserver, Lacewing::Pump &_Pump)
            : Webserver (_Webserver), Pump (_Pump), Timer (_Pump)
    {
        Socket = SecureSocket = 0;

        memset (&Handlers, 0, sizeof (Handlers));

        AutoFinish = true;
        Sessions = 0;

        Timeout = 5;

        Timer.Tag = this;
        Timer.onTick (TimerTickStatic);
    }

    static void TimerTickStatic (Lacewing::Timer &);
    void TimerTick ();
};

class WebserverClient;

struct Webserver::Request::Internal : public Webserver::Request
{
    void * Tag;

    Webserver::Internal &Server;
    WebserverClient &Client;

    Internal (Webserver::Internal &, WebserverClient &);
    ~ Internal ();

    void Clean ();

    struct Cookie
    {
        char * Name;
        char * Value;
        char * Attr;

        bool Changed;

        UT_hash_handle hh;

    } * Cookies;

    void SetCookie (size_t name_len, const char * name,
                    size_t value_len, const char * value,
                    size_t attr_len, const char * attr, bool changed);

    /* Input */

    char Version_Major, Version_Minor;

    char Method     [16];
    char URL        [4096];
    char Hostname   [128];

    List <WebserverHeader> InHeaders;
    lw_nvhash * GetItems, * PostItems;

    bool In_Version (size_t len, const char * version);

    bool In_Method (size_t len, const char * method);

    bool In_Header (size_t name_len, const char * name,
                    size_t value_len, const char * value);

    bool In_URL (size_t len, const char * URL);
    
    /* The protocol implementation can use this for any intermediate
     * buffering, providing it contains the request body (if any) when
     * the handler is called.
     */

    HeapBuffer Buffer;

    void ParsePostData ();
    bool ParsedPostData;


    /* Output */

    char Status [64];

    List <WebserverHeader> OutHeaders;
    
    void BeforeHandler ();
    void AfterHandler ();

    void RunStandardHandler ();    

    bool Responded;
    void Respond ();
};

class WebserverClient : public Stream
{
public:

    bool Secure;

    Lacewing::Server::Client &Socket;
    Webserver::Internal &Server;

    int Timeout;

    WebserverClient (Webserver::Internal &, Lacewing::Server::Client &, bool Secure);
    virtual ~ WebserverClient ();

    virtual void Tick () = 0;

    virtual void Respond (Webserver::Request::Internal &Request) = 0;

    struct Multipart * Multipart;
};

#include "http/HTTP.h"

#ifndef LacewingNoSPDY
    #include "spdy/SPDY.h"
#endif

