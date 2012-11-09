
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

size_t lw_stream_bytes_left (lw_stream * stream)
{
    return ((Stream *) stream)->BytesLeft ();
}

void lw_stream_read (lw_stream * stream, size_t bytes)
{
    ((Stream *) stream)->Read (bytes);
}

void lw_stream_begin_queue (lw_stream * stream)
{
    ((Stream *) stream)->BeginQueue ();
}

size_t lw_stream_queued (lw_stream * stream)
{
    return ((Stream *) stream)->Queued ();
}

void lw_stream_end_queue
        (lw_stream * stream, int head_buffers,
            const char ** buffers, size_t * lengths)
{ 
    ((Stream *) stream)->EndQueue (head_buffers, buffers, lengths);
}

void lw_stream_clear_queue (lw_stream * stream)
{
    ((Stream *) stream)->ClearQueue ();
}

void lw_stream_write (lw_stream * stream, const char * buffer, size_t length)
{
    ((Stream *) stream)->Write (buffer, length);
}

size_t lw_stream_write_partial (lw_stream * stream, const char * buffer, size_t length)
{
    return ((Stream *) stream)->WritePartial (buffer, length);
}

void lw_stream_write_text (lw_stream * stream, const char * buffer)
{
    ((Stream *) stream)->Write (buffer);
}

void lw_stream_write_stream
    (lw_stream * stream, lw_stream * src, size_t size, lw_bool delete_when_finished)
{
    ((Stream *) stream)->Write (*(Stream *) src, size, delete_when_finished ? true : false);
}

void lw_stream_write_file (lw_stream * stream, const char * filename)
{
    ((Stream *) stream)->Write (filename);
}
 
void lw_stream_add_filter_upstream
        (lw_stream * stream, lw_stream * filter, lw_bool delete_with_stream)
{
    ((Stream *) stream)->AddFilterUpstream (*(Stream *) filter, delete_with_stream ? true : false);
}

void lw_stream_add_filter_downstream
        (lw_stream * stream, lw_stream * filter, lw_bool delete_with_stream)
{
    ((Stream *) stream)->AddFilterDownstream (*(Stream *) filter, delete_with_stream ? true : false);
}

lw_bool lw_stream_is_transparent (lw_stream * stream)
{
    return ((Stream *) stream)->IsTransparent ();
}

void * lw_stream_cast (lw_stream * stream, void * type)
{
    return ((Stream *) stream)->Cast (type);
}

void lw_stream_add_handler_data
        (lw_stream * stream, lw_stream_handler_data proc, void * tag)
{
    ((Stream *) stream)->AddHandlerData ((Stream::HandlerData) proc, tag);
}

void lw_stream_remove_handler_data
        (lw_stream * stream, lw_stream_handler_data proc, void * tag)
{
    ((Stream *) stream)->RemoveHandlerData ((Stream::HandlerData) proc, tag);
}

void lw_stream_close (lw_stream * stream, lw_bool immediate)
{
    ((Stream *) stream)->Close (immediate);
}

void lw_stream_add_handler_close
        (lw_stream * stream, lw_stream_handler_data proc, void * tag)
{
    ((Stream *) stream)->AddHandlerClose ((Stream::HandlerClose) proc, tag);
}

void lw_stream_remove_handler_close
        (lw_stream * stream, lw_stream_handler_data proc, void * tag)
{
    ((Stream *) stream)->RemoveHandlerClose ((Stream::HandlerClose) proc, tag);
}


void lw_stream_writef (lw_stream * stream, const char * format, ...)
{
    va_list args;
    va_start (args, format);
    
    char * data;
    ssize_t size = lwp_format (&data, format, args);
    
    if (size > 0)
        ((Stream *) stream)->Write (data, size);

    va_end (args);
}

void lw_stream_delete (lw_stream * stream)
{
    delete ((Stream *) stream);
}


