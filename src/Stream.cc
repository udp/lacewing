
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

#include "lw_common.h"
#include "Stream.h"

Stream::Stream ()
{
    internal = new Internal (this);

    internal->Graph->Expand ();
}

Stream::Stream (Lacewing::Pump &Pump)
{
    internal = new Internal (this);

    internal->Pump = &Pump;

    internal->Graph->Expand ();
}

Stream::~ Stream ()
{
    internal->DataHandlers.Clear ();

    internal->Graph->ClearExpanded ();

    Close (true);

    /* Prevent any entry to Close now */

    internal->Flags |= Internal::Flag_Closing;


    /* If we are a root in the graph, remove us */

    internal->Graph->Roots.Remove (internal);


    /* If we are filtering any other streams, remove us from their filter list */

    while (internal->Filtering.Size)
    {
        Internal::Filter * filter = (** internal->Filtering.First);
        internal->Filtering.PopFront ();

        filter->StreamPtr->FiltersUpstream.Remove (filter);
        filter->StreamPtr->FiltersDownstream.Remove (filter);

        delete filter;
    }

    
    /* If we are being filtered upstream by any other streams, remove us from
     * their filtering list.
     */

    while (internal->FiltersUpstream.Size)
    {
        Internal::Filter * filter = (** internal->FiltersUpstream.First);
        internal->FiltersUpstream.PopFront ();

        filter->FilterPtr->Filtering.Remove (filter);

        if (filter->DeleteWithStream)
            delete &filter->FilterPtr->Public;

        delete filter;
    }


    /* If we are being filtered downstream by any other streams, remove us from
     * their filtering list.
     */

    while (internal->FiltersDownstream.Size)
    {
        Internal::Filter * filter = (** internal->FiltersDownstream.First);
        internal->FiltersDownstream.PopFront ();

        filter->FilterPtr->Filtering.Remove (filter);

        if (filter->DeleteWithStream)
            delete &filter->FilterPtr->Public;

        delete filter;
    }

    /* Is the graph empty now? */

    if (internal->Graph->Roots.Size == 0)
        internal->Graph->Delete ();
    else
        internal->Graph->Expand ();


    /* TODO : clear queues */

    internal->Public = 0;

    /* The stream has now been detached from everything, but if some other
     * routine is in progress, we'll leave the internal structure around
     * for that to delete later.
     */

    if (internal->UserCount == 0)
        delete internal;
}

Stream::Internal::Internal (Stream * _Public) : Public (_Public)
{
    UserCount = 0;

    Flags = 0;
    Retry = Stream::Retry_Never;

    HeadUpstream = 0;
    PrevDirect = 0;

    Graph = new (std::nothrow) StreamGraph ();

    Graph->Roots.Push (this);

    LastExpand = Graph->LastExpand;
}

Stream::Internal::~ Internal ()
{
    if (Public)
        Public->Close ();
}

void Stream::Write (const char * buffer, size_t size)
{
    internal->Write (buffer, size, 0);
}

size_t Stream::WritePartial (const char * buffer, size_t size)
{
    return internal->Write (buffer, size, Internal::Write_Partial);
}

size_t Stream::Internal::Write (const char * buffer, size_t size, int flags)
{
    if (size == -1)
        size = strlen (buffer);

    lwp_trace ("Writing " lwp_fmt_size " bytes", size);

    if (size == 0)
        return size; /* nothing to do */

    if ((! (flags & Write_IgnoreFilters)) && HeadUpstream)
    {
        lwp_trace ("%p is filtered upstream by %p; writing " lwp_fmt_size " to that", this, HeadUpstream, size);

        /* There's a filter to write the data to first.  At the end of the
         * chain of filters, the data will be written back to us again
         * with the Write_IgnoreFilters flag.
         */

        if (flags & Write_Partial)
            return HeadUpstream->Write (buffer, size, Write_Partial);

        HeadUpstream->Write (buffer, size, 0);
        return size;
    }

    if (Prev.Size)
    {
        if (! (flags & Write_IgnoreBusy))
        {
            lwp_trace ("Busy: Adding to back queue");

            if (flags & Write_Partial)
                return 0;

            /* Something is behind us, but this data doesn't come from it.
             * Queue the data to write when we're not busy.
             */

            if ( (!BackQueue.Size) || (** BackQueue.Last).Type != Queued::Type_Data)
                (** BackQueue.Push ()).Type = Queued::Type_Data;

            (** BackQueue.Last).Buffer.Add (buffer, size);

            return size;
        }

        /* Something is behind us and gave us this data. */

        if ((! (flags & Write_IgnoreQueue)) && FrontQueue.Size)
        {
            lwp_trace ("%p : Adding to front queue (Queueing = %d, FrontQueue.Size = %d)",
                            this, (int) ( (Flags & Flag_Queueing) != 0), FrontQueue.Size);

            if (flags & Write_Partial)
                return 0;

            if ( (!FrontQueue.Size) || (** FrontQueue.Last).Type != Queued::Type_Data)
                (** FrontQueue.Push ()).Type = Queued::Type_Data;

            (** FrontQueue.Last).Buffer.Add (buffer, size);

            if (Retry == Stream::Retry_MoreData)
                Public->Retry ();

            return size;
        }

        if (Public->IsTransparent ())
        {
            Data (buffer, size);
            return size;
        }

        size_t written = Public->Put (buffer, size);

        lwp_trace ("%p : Put wrote " lwp_fmt_size " of " lwp_fmt_size, this, written, size);

        if (flags & Write_Partial)
            return written;

        if (written < size)
        {
            if ( (!FrontQueue.Size) || (** FrontQueue.Last).Type != Queued::Type_Data)
                (** FrontQueue.Push ()).Type = Queued::Type_Data;

            (** FrontQueue.Last).Buffer.Add
                    (buffer + written, size - written);
        }

        return size;
    }

    if ( (! (flags & Write_IgnoreQueue)) && ( (Flags & Flag_Queueing) || BackQueue.Size))
    {
        lwp_trace ("%p : Adding to back queue (Queueing = %d, FrontQueue.Size = %d)",
                this, (int) ( (Flags & Flag_Queueing) != 0), FrontQueue.Size);

        if (flags & Write_Partial)
            return 0;

        if ( (!BackQueue.Size) || (** BackQueue.Last).Type != Queued::Type_Data)
            (** BackQueue.Push ()).Type = Queued::Type_Data;

        (** BackQueue.Last).Buffer.Add (buffer, size);

        if (Retry == Stream::Retry_MoreData)
            Public->Retry ();

        return size;
    }

    if (Public->IsTransparent ())
    {
        Data (buffer, size);
        return size;
    }

    size_t written = Public->Put (buffer, size);

    if (flags & Write_Partial)
        return written;

    if (written < size)
    {
        if ( (!BackQueue.Size) || (** BackQueue.Last).Type != Queued::Type_Data)
            (** BackQueue.Push ()).Type = Queued::Type_Data;

        if (flags & Write_IgnoreQueue)
        {
            Queued &first = (** BackQueue.First);

            if (!first.Buffer.Size)
                first.Buffer.Add (buffer + written, size - written);
            else
            {
                /* TODO : rewind offset where possible instead of creating a new Queued? */

                Queued &queued = (** BackQueue.PushFront ());
                
                queued.Type = Queued::Type_Data;
                queued.Buffer.Add (buffer + written, size - written);
            }
        }
        else
            (** BackQueue.Last).Buffer.Add (buffer + written, size - written);
    }

    return size;
}

void Stream::Write (Stream &stream, size_t size, bool delete_when_finished)
{
    if (size == 0)
    {
        if (delete_when_finished)
            delete &stream;
        
        return;
    }

    internal->Write (stream.internal, size,
            delete_when_finished ? Internal::Write_DeleteStream : 0);
}

void Stream::Internal::Write (Stream::Internal * stream, size_t size, int flags)
{
    if ( ( (! (flags & Write_IgnoreQueue)) && (BackQueue.Size || Flags & Flag_Queueing))
         || ( (! (flags & Write_IgnoreBusy)) && Prev.Size))
    {
        Queued &queued = ** BackQueue.Push ();

        queued.Type = Queued::Type_Stream;

        queued.StreamPtr = stream;
        queued.StreamBytesLeft = size;

        if (flags & Write_DeleteStream)
            queued.Flags |= Queued::Flag_DeleteStream;

        return;
    }

    /* Are we currently in a different graph from the source stream? */

    if (stream->Graph != Graph)
        stream->Graph->Swallow (Graph);

    assert (stream->Graph == Graph);

    StreamGraph::Link * link = new (std::nothrow) StreamGraph::Link
                (stream, 0, this, 0, size, flags & Write_DeleteStream);

    stream->Next.Push (link);
    Prev.Push (link);

    /* This stream is now linked to, so doesn't need to be a root */

    Graph->Roots.Remove (this);

    Graph->Expand ();
    Graph->Read ();
}

void Stream::WriteFile (const char * filename)
{
    /* This method may only be used when the stream is associated with a pump */

    assert (internal->Pump);

    File * file = new File (*internal->Pump, filename, "r");

    if (!file->Valid ())
        return;

    Write (*file, file->BytesLeft (), true);
}

void Stream::AddFilterUpstream
    (Stream &filter, bool delete_with_stream, bool close_together)
{
    Internal::Filter * _filter = new (std::nothrow) Internal::Filter;

    _filter->StreamPtr = internal;
    _filter->FilterPtr = filter.internal;
    _filter->DeleteWithStream = delete_with_stream;
    _filter->CloseTogether = close_together;

    /* Upstream data passes through the most recently added filter first */

    internal->FiltersUpstream.PushFront (_filter);
    filter.internal->Filtering.Push (_filter);

    if (filter.internal->Graph != internal->Graph)
        internal->Graph->Swallow (filter.internal->Graph);

    internal->Graph->Expand ();
    internal->Graph->Read ();
}

void Stream::AddFilterDownstream
    (Stream &filter, bool delete_with_stream, bool close_together)
{
    Internal::Filter * _filter = new (std::nothrow) Internal::Filter;

    _filter->StreamPtr = internal;
    _filter->FilterPtr = filter.internal;
    _filter->DeleteWithStream = delete_with_stream;
    _filter->CloseTogether = close_together;

    /* Downstream data passes through the most recently added filter last */

    internal->FiltersDownstream.Push (_filter);
    filter.internal->Filtering.Push (_filter);

    if (filter.internal->Graph != internal->Graph)
        internal->Graph->Swallow (filter.internal->Graph);

    internal->Graph->Expand ();
    internal->Graph->Read ();
}

size_t Stream::BytesLeft ()
{
    return -1;
}

void Stream::Read (size_t bytes)
{
    lwp_trace ("Base stream Read called? (" lwp_fmt_size ")", bytes);
}

void Stream::Internal::Data (const char * buffer, size_t size)
{
    int num_data_handlers = ExpDataHandlers.Size, i = 0;

    ++ UserCount;

    DataHandler * data_handlers = (DataHandler *)
        alloca (ExpDataHandlers.Size * sizeof (DataHandler));

    for (List <DataHandler>::Element * E
                = ExpDataHandlers.First; E; E = E->Next)
    {
        DataHandler &handler = (** E);

        data_handlers [i ++] = handler;

        ++ handler.StreamPtr->UserCount;
    }

    for (int i = 0; i < num_data_handlers; ++ i)
    {
        DataHandler &handler = data_handlers [i];

        if (handler.StreamPtr->Public)
            handler.Proc (*handler.StreamPtr->Public, handler.Tag, buffer, size);

        if ((-- handler.StreamPtr->UserCount) == 0)
            delete handler.StreamPtr;
    }

    /* Write the data to any streams next in the (expanded) graph, if this
     * stream still exists.
     */

    if (Public)
    {
        Push (buffer, size);

        /* Pushing data may result in stream destruction */

        if ((-- UserCount) == 0 && !Public)
        {
            delete this;
            return;
        }
    }
}

void Stream::Data (const char * buffer, size_t size)
{
    internal->Data (buffer, size);
}

void Stream::Internal::Push (const char * buffer, size_t size)
{
    int num_links = NextExpanded.Size;

    if (!num_links)
        return; /* nothing to do */

    ++ UserCount;

    StreamGraph::Link ** links = (StreamGraph::Link **)
        alloca (num_links * sizeof (StreamGraph::Link *));

    int i = 0;

    /* Copy the link dest pointers into our local array */

    for (List <StreamGraph::Link *>::Element * E
                = NextExpanded.First; E; E = E->Next)
    {
        links [i ++] = (** E);
    }

    int LastExpand = Graph->LastExpand;

    for (i = 0; i < num_links; ++ i)
    {
        StreamGraph::Link * link = links [i];

        if (!link)
        {
            lwp_trace ("Link %d in the local Push list is now dead; skipping", i);
            continue;
        }

        lwp_trace ("Pushing to link %d of %d (%p)", i, num_links, link);

        size_t to_write = link->BytesLeft != -1 &&
                            size > link->BytesLeft ? link->BytesLeft : size;

        if (!link->ToExp->IsTransparent ())
        {
            /* If we have some actual data, write it to the target stream */

            lwp_trace ("Link target is not transparent; buffer = %p", buffer);

            if (buffer)
            {
                link->ToExp->Write
                    (buffer, to_write, Write_IgnoreFilters | Write_IgnoreBusy);
            }
        }
        else
        {
            lwp_trace ("Link target is transparent; pushing data forward");

            /* Target stream is transparent - have it push the data forward */

            link->ToExp->Push (buffer, to_write);
        }

        /* Pushing data may have caused this stream to be deleted */

        if (!Public)
            break;

        if (Graph->LastExpand != LastExpand)
        {
            lwp_trace ("Graph has re-expanded - checking local Push list...");

            /* If any new links have been added to NextExpanded, this Push will
             * not write to them.  However, if any links in our local array
             * (yet to be written to) have disappeared, they must be removed.
             */

            for (int x = i; x < num_links; ++ x)
            {
                if (!NextExpanded.Find (links [x]))
                {
                    if (link == links [x])
                        link = 0;

                    links [x] = 0;
                }
            }

            if (!link)
                continue;
        }

        if (!link->To)
            continue; /* filters cannot expire */

        if (link->BytesLeft == -1)
        {
            if (link->FromExp->Public->BytesLeft () != 0)
                continue;
        }
        else
        {
            if ((link->BytesLeft -= to_write) > 0)
                continue;
        }

        if (link->DeleteStream)
        {
            delete link->From->Public; /* will re-expand the graph */
        }
        else
        {
            Graph->ClearExpanded ();

            Next.Remove (link);
            link->To->Prev.Remove (link);

            /* Since the target and anything after it are still part
             * of this graph, make it a root before deleting the link.
             */

            Graph->Roots.Push (link->To);

            Graph->Expand ();

            delete link;

            Graph->Read ();
        }

        /* Maybe deleting the source stream caused this stream to be
         * deleted, too?
         */

        if (!Public)
            break;

        lwp_trace ("Graph re-expanded by Push - checking local list...");

        /* Since the graph has re-expanded, check the rest of the links we
         * intend to write to are still present in the list.
         */

        for (int x = i; x < num_links; ++ x)
            if (!NextExpanded.Find (links [x]))
                links [x] = 0;
    }

    if ((-- UserCount) == 0 && (!Public))
        delete this;

    if (Flags & Flag_CloseASAP && MayClose ())
        Close (true);
}

size_t Stream::Put (Stream &, size_t size)
{
    return -1;
}

void Stream::Retry (int when)
{
    lwp_trace ("Stream::Retry for %p (PrevDirect %p)", internal, internal->PrevDirect);

    if (when == Retry_Now)
    {
        if (!internal->WriteDirect ())
        {
            /* TODO: ??? */
        }

        internal->WriteQueued ();

        return;
    }

    internal->Retry = when;
}

void Stream::Internal::WriteQueued ()
{
    if (Flags & Flag_Queueing)
        return;

    lwp_trace ("%p : Writing FrontQueue (size = %d)", this, FrontQueue.Size);

    WriteQueued (FrontQueue);

    lwp_trace ("%p : FrontQueue size is now %d, Prev.Size is %d, %d in back queue",
                    this, FrontQueue.Size, Prev.Size, BackQueue.Size);

    if (FrontQueue.Size == 0 && !Prev.Size)
        WriteQueued (BackQueue);

    if (Flags & Flag_CloseASAP && MayClose ())
        Close (true);
}

void Stream::Internal::WriteQueued (List <Queued> &queue)
{
    lwp_trace ("%p : WriteQueued : %d to write", this, queue.Size);

    while (queue.Size)
    {
        Queued &queued = ** queue.First;

        if (queued.Type == Queued::Type_BeginMarker)
        {
            queue.PopFront ();
            Flags |= Flag_Queueing;

            return;
        }

        if (queued.Type == Queued::Type_Data)
        {
            if ((queued.Buffer.Size - queued.Buffer.Offset) > 0)
            {
                /* There's still something in the buffer that needs to be written */

                size_t written = Write (queued.Buffer.Buffer + queued.Buffer.Offset,
                                          queued.Buffer.Size - queued.Buffer.Offset,
                                                Write_IgnoreQueue | Write_Partial |
                                                      Write_IgnoreBusy);

                if ((queued.Buffer.Offset += written) < queued.Buffer.Size)
                    break; /* couldn't write everything */

                queued.Buffer.Reset ();
            }

            queue.PopFront ();

            continue;
        }

        if (queued.Type == Queued::Type_Stream)
        {
            Stream::Internal * stream = queued.StreamPtr;
            size_t bytes = queued.StreamBytesLeft;

            int flags = Write_IgnoreQueue;

            if (queued.Flags & Queued::Flag_DeleteStream)
                flags |= Write_DeleteStream;

            queue.PopFront ();

            Write (stream, bytes, flags);

            continue;
        }
    }
}

bool Stream::Internal::WriteDirect ()
{
    if (!PrevDirect)
        return true;

    size_t written = Public->Put (*PrevDirect->Public, DirectBytesLeft);

    if (written != -1)
    {
        PrevDirect->Push (0, written);

        if (DirectBytesLeft != -1)
            DirectBytesLeft -= written;

        return true;
    }

    return false;
}

bool Stream::Internal::MayClose ()
{
    return Prev.Size == 0 && BackQueue.Size == 0 && FrontQueue.Size == 0;
}

bool Stream::Internal::Close (bool immediate)
{
    if (Flags & Flag_Closing)
        return false;

    if ( (!immediate) && !MayClose ())
    {
        Flags |= Flag_CloseASAP;
        return false;
    }

    Flags |= Flag_Closing;


    ++ UserCount;


    /* If RootsExpanded is already empty, something else has already cleared
     * the expanded graph (e.g. another stream closing) and should re-expand
     * later (meaning we don't have to bother)
     */

    bool already_cleared = (Graph->RootsExpanded.Size == 0);

    if (!already_cleared)
        Graph->ClearExpanded ();


    /* Anything that comes before us can no longer link here */

    for (List <StreamGraph::Link *>::Element * E = Prev.First;
                E; E = E->Next)
    {
        StreamGraph::Link * link = (** E);

        link->From->Next.Remove (link);

        delete link;
    }

    Prev.Clear ();


    /* Anything that comes after us will have to be a root */

    for (List <StreamGraph::Link *>::Element * E = Next.First;
                E; E = E->Next)
    {
        StreamGraph::Link * link = (** E);

        Graph->Roots.Push (link->To);

        link->To->Prev.Remove (link);

        delete link;
    }

    Next.Clear ();


    /* If we're set to close together with any filters, close those too (this
     * is the reason for the already_cleared check)
     */

    Stream::Internal ** to_close =
        (Stream::Internal **) alloca (sizeof (Stream::Internal *) *
         (Filtering.Size + FiltersUpstream.Size + FiltersDownstream.Size));

    int n = 0;

    for (List <Filter *>::Element * E = Filtering.First; E; E = E->Next)
    {
        Filter * filter = (** E);

        if (filter->CloseTogether)
        {
            ++ filter->StreamPtr->UserCount;
            to_close [n ++] = filter->StreamPtr;
        }
    }

    for (List <Filter *>::Element * E = FiltersUpstream.First; E; E = E->Next)
    {
        Filter * filter = (** E);

        if (filter->CloseTogether)
        {
            ++ filter->FilterPtr->UserCount;
            to_close [n ++] = filter->FilterPtr;
        }
    }

    for (List <Filter *>::Element * E = FiltersDownstream.First; E; E = E->Next)
    {
        Filter * filter = (** E);

        if (filter->CloseTogether)
        {
            ++ filter->FilterPtr->UserCount;
            to_close [n ++] = filter->FilterPtr;
        }
    }

    for (int i = 0; i < n; ++ i)
    {
        Stream::Internal * stream = to_close [i];

        -- stream->UserCount;

        if (!stream->Public)
        {
            if (stream->UserCount == 0)
                delete stream;
        }
        else
        {
            stream->Public->Close ();
        }
    }


    if (!already_cleared)
    {
        if (!Graph->Roots.Find (this))
            Graph->Roots.Push (this);

        Graph->Expand ();

        Graph->Read ();
    }


    for (List <Internal::CloseHandler>::Element * E =
            CloseHandlers.First; E; E = E->Next)
    {
        (** E).Proc (*Public, (** E).Tag);

        if (!Public)
            break;  /* close handler destroyed the stream */
    }

    Flags &= ~ Flag_Closing;
    
    if ((-- UserCount) == 0 && !Public) /* see Stream destructor */
        delete this;

    return true;
}

bool Stream::Close (bool immediate)
{
    return internal->Close (immediate);
}

void Stream::BeginQueue ()
{
    if (internal->BackQueue.Size || internal->FrontQueue.Size)
    {
        /* Although we're going to start queueing any new data, whatever is
         * currently in the queue still needs to be written.  A Queued with
         * the BeginMarker type indicates where the stream should stop writing
         * and set the Queueing flag
         */

        (** internal->BackQueue.Push ()).Type = Internal::Queued::Type_BeginMarker;
    }
    else
    {
        internal->Flags |= Internal::Flag_Queueing;
    }
}

size_t Stream::Queued ()
{
    size_t size = 0;

    for (List <Internal::Queued>::Element * E
                = internal->BackQueue.First; E; E = E->Next)
    {
        Internal::Queued &queued = ** E;

        if (queued.Type == Internal::Queued::Type_Data)
        {
            size += queued.Buffer.Size - queued.Buffer.Offset;
            continue;
        }

        if (queued.Type == Internal::Queued::Type_Stream)
        {
            Stream::Internal * stream = queued.StreamPtr;

            if (!stream)
                continue;

            if (queued.StreamBytesLeft != -1)
            {
                size += (** E).StreamBytesLeft;
                continue;
            }

            size_t bytes = stream->Public->BytesLeft ();

            if (bytes == -1)
                return -1;

            size += bytes;
            
            continue;
        }
    }

    return size;
}

void Stream::EndQueue
        (int head_buffers, const char ** buffers, size_t * lengths)
{
    for (int i = 0; i < head_buffers; ++ i)
    {
        internal->Write (buffers [i], lengths [i],
                    Internal::Write_IgnoreQueue | Internal::Write_IgnoreBusy);
    }

    EndQueue ();
}

void Stream::EndQueue ()
{
    lwp_trace ("%p : EndQueue", internal);

    /* TODO : Look for a queued item w/ Flag_BeginQueue if Queueing is false? */

    assert (internal->Flags & Internal::Flag_Queueing);

    internal->Flags &= ~ Internal::Flag_Queueing;

    internal->WriteQueued ();
}

void Stream::ClearQueue ()
{
    while (internal->BackQueue.Size > 0)
    {
        Internal::Queued &queued = (** internal->BackQueue.First);

        if (queued.Type == Internal::Queued::Type_Stream &&
                queued.Flags & Internal::Queued::Flag_DeleteStream)
        {
            ++ internal->UserCount;

            delete queued.StreamPtr;

            if ((-- internal->UserCount) == 0 && !internal->Public)
            {
                delete this;
                return;
            }
        }

        internal->BackQueue.PopFront ();
    }
}

bool Stream::Internal::IsTransparent ()
{
    assert (Public);

    return (!ExpDataHandlers.Size) && (BackQueue.Size + FrontQueue.Size) == 0
                    && (! (Flags & Flag_Queueing)) && Public->IsTransparent ();
}

bool Stream::IsTransparent ()
{
    return false;
}

void * Stream::Cast (void *)
{
    return 0;
}

void Stream::AddHandlerData (HandlerData Proc, void * Tag)
{   
    /* TODO : Prevent the same handler being registered twice? */

    Internal::DataHandler handler = { Proc, internal, Tag };
    internal->DataHandlers.Push (handler);

    internal->Graph->ClearExpanded ();
    internal->Graph->Expand ();
} 

void Stream::RemoveHandlerData (HandlerData Proc, void * Tag)
{   
    for (List <Internal::DataHandler>::Element * E =
            internal->DataHandlers.First; E; E = E->Next)
    {
        if ((** E).Proc == Proc)
        {
            internal->DataHandlers.Erase (E);
            break;
        }
    }

    internal->Graph->ClearExpanded ();
    internal->Graph->Expand ();
}

void Stream::AddHandlerClose (HandlerClose Proc, void * Tag)
{   
    Internal::CloseHandler handler = { Proc, Tag };
    internal->CloseHandlers.Push (handler);
} 

void Stream::RemoveHandlerClose (HandlerClose Proc, void * Tag)
{   
    for (List <Internal::CloseHandler>::Element * E =
            internal->CloseHandlers.First; E; E = E->Next)
    {
        if ((** E).Proc == Proc)
        {
            internal->CloseHandlers.Erase (E);
            break;
        }
    }
}



