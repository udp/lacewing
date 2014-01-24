
/* vim: set et ts=3 sw=3 ft=c:
 *
 * Copyright (C) 2012, 2013 James McLaughlin et al.  All rights reserved.
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

#include "common.h"
#include "stream.h"

void lwp_stream_init (lw_stream ctx, const lw_streamdef * def, lw_pump pump)
{
   lwp_trace ("Stream %p created with def %p", ctx, def);

   memset (ctx, 0, sizeof (*ctx));

   /* This means that the refcount can never become 0 until lw_stream_delete */
   lwp_retain (ctx, "lwp_stream_init"); 

   ctx->def = def;
   ctx->pump = pump;

   ctx->retry = lw_stream_retry_never;

   ctx->graph = lwp_streamgraph_new ();
   list_push (ctx->graph->roots, ctx);

   ctx->last_expand = ctx->graph->last_expand;

   lwp_streamgraph_expand (ctx->graph);
}

lw_stream lw_stream_new (const lw_streamdef * def, lw_pump pump)
{
   lw_stream ctx = (lw_stream) calloc (sizeof (*ctx) + def->tail_size, 1);
   lwp_stream_init (ctx, def, pump);

   return ctx;
}

const lw_streamdef * lw_stream_get_def (lw_stream ctx)
{
   return ctx->def;
}

void * lw_stream_tail (lw_stream ctx)
{
   return ctx + 1;
}

lw_stream lw_stream_from_tail (void * tail)
{
   return ((lw_stream) tail) - 1;
}

void lw_stream_delete (lw_stream ctx)
{
   if (!ctx)
      return;

   if (ctx->flags & lwp_stream_flag_dead)
      return;

   ctx->flags |= lwp_stream_flag_dead;

   list_clear (ctx->data_hooks);

   lwp_streamgraph_clear_expanded (ctx->graph);

   lw_stream_close (ctx, lw_true);

   /* Prevent any entry to lw_stream_close now */

   ctx->flags |= lwp_stream_flag_closing;
   
   list_clear (ctx->close_hooks);

   /* If this stream is a root in the graph, remove it */

   list_remove (ctx->graph->roots, ctx);

   /* If this stream is filtering any other streams, remove it from their
    * filter list.
    */

   while (list_length (ctx->filtering) > 0)
   {
      lwp_stream_filterspec spec = list_front (ctx->filtering);
      list_pop_front (ctx->filtering);

      list_remove (spec->stream->filters_upstream, spec);
      list_remove (spec->stream->filters_downstream, spec);

      free (spec);
   }

   /* If this stream is being filtered upstream by any other streams, remove
    * it from their filtering list.
    */

   while (list_length (ctx->filters_upstream) > 0)
   {
      lwp_stream_filterspec spec = list_front (ctx->filters_upstream);
      list_pop_front (ctx->filters_upstream);

      list_remove (spec->filter->filtering, spec);

      if (spec->delete_with_stream)
         lw_stream_delete (spec->filter);

      free (spec);
   }

   /* If this stream is being filtered downstream by any other streams, remove
    * it from their filtering list.
    */

   while (list_length (ctx->filters_downstream) > 0)
   {
      lwp_stream_filterspec spec = list_front (ctx->filters_downstream);
      list_pop_front (ctx->filters_downstream);

      list_remove (spec->filter->filtering, spec);

      if (spec->delete_with_stream)
         lw_stream_delete (spec->filter);

      free (spec);
   }

   /* Is the graph empty now? */

   if (list_length (ctx->graph->roots) == 0)
      lwp_streamgraph_delete (ctx->graph);
   else
      lwp_streamgraph_expand (ctx->graph);

   ctx->graph = 0;

   if (ctx->def->cleanup)
      ctx->def->cleanup (ctx);

   /* Clear queues */

   list_each (ctx->front_queue, queued)
      lwp_heapbuffer_free (&queued.buffer);

   list_each (ctx->back_queue, queued)
      lwp_heapbuffer_free (&queued.buffer);

   list_clear (ctx->front_queue);
   list_clear (ctx->back_queue);


   /* This matches the lwp_retain in lw_stream_new, allowing the refcount to
    * become 0 and the stream to be destroyed.
    */
   lwp_release (ctx, "lwp_stream_init");
}

/* The public lw_stream_write just calls lwp_stream_write with flags = 0 */

void lw_stream_write (lw_stream ctx, const char * buffer, size_t size)
{
   lwp_stream_write (ctx, buffer, size, 0);
}

void lw_stream_writef (lw_stream ctx, const char * format, ...)
{
   va_list args;
   va_start (args, format);

   lw_stream_writev (ctx, format, args);

   va_end (args);
}

void lw_stream_writev (lw_stream ctx, const char * format, va_list args)
{
   char * data = 0;
   size_t size = lwp_format (&data, format, args);

   if (size > 0)
      lw_stream_write (ctx, data, size);

   free (data);
}


/* Convenience queue functions for lwp_stream_write */

static void queue_back (lw_stream ctx, const char * buffer, size_t size)
{
   if ( (!list_length (ctx->back_queue)) ||
         list_back (ctx->back_queue).type != lwp_stream_queued_data)
   {
      struct _lwp_stream_queued queued = {};

      queued.type = lwp_stream_queued_data;

      list_push (ctx->back_queue, queued);
   }

   lwp_heapbuffer_add (&list_elem_back (ctx->back_queue)->buffer, buffer, size);
}

static void queue_front (lw_stream ctx, const char * buffer, size_t size)
{
   if ( (!list_length (ctx->front_queue)) ||
         list_back (ctx->front_queue).type != lwp_stream_queued_data)
   {
      struct _lwp_stream_queued queued = {};

      queued.type = lwp_stream_queued_data;

      list_push (ctx->front_queue, queued);
   }

   lwp_heapbuffer_add (&list_elem_back (ctx->front_queue)->buffer, buffer, size);
}

size_t lwp_stream_write (lw_stream ctx, const char * buffer, size_t size, int flags)
{
   if (size == -1)
      size = strlen (buffer);

   lwp_trace ("Writing " lwp_fmt_size " bytes to %p with flags %d", size, ctx, flags);

   if (size == 0)
      return size; /* nothing to do */

   if ((! (flags & lwp_stream_write_ignore_filters)) && ctx->head_upstream)
   {
      lwp_trace ("%p is filtered upstream by %p; writing " lwp_fmt_size " to that",
                     ctx, ctx->head_upstream, size);

      /* There's a filter to write the data to first.  At the end of the
       * chain of filters, the data will be written back to us again
       * with the write_ignore_filters flag.
       */

      if (flags & lwp_stream_write_partial)
      {
         return lwp_stream_write (ctx->head_upstream, buffer, size,
                                  lwp_stream_write_partial);
      }

      lwp_stream_write (ctx->head_upstream, buffer, size, 0);

      return size;
   }

   if (list_length (ctx->prev) > 0)
   {
      if (! (flags & lwp_stream_write_ignore_busy))
      {
         lwp_trace ("Busy: Adding to back queue");

         if (flags & lwp_stream_write_partial)
            return 0;

         /* Something is behind us, but this data doesn't come from it.
          * Queue the data to write when we're not busy.
          */

         queue_back (ctx, buffer, size);

         return size;
      }

      /* Something is behind us and gave us this data. */

      if ((! (flags & lwp_stream_write_ignore_queue))
                && list_length (ctx->front_queue) > 0)
      {
         lwp_trace ("%p : Adding to front queue (queueing = %d, front queue length = %d)",
               ctx, (int) ( (ctx->flags & lwp_stream_flag_queueing) != 0),
               list_length (ctx->front_queue));

         if (flags & lwp_stream_write_partial)
            return 0;

         queue_front (ctx, buffer, size);

         if (ctx->retry == lw_stream_retry_more_data)
            lw_stream_retry (ctx, lw_stream_retry_now);

         return size;
      }

      if (ctx->def->is_transparent && ctx->def->is_transparent (ctx))
      {
         lw_stream_data (ctx, buffer, size);
         return size;
      }

      size_t written = ctx->def->sink_data ?
         ctx->def->sink_data (ctx, buffer, size) : size;

      lwp_trace ("%p : Stream sank " lwp_fmt_size " of " lwp_fmt_size,
                     ctx, written, size);

      if (flags & lwp_stream_write_partial)
         return written;

      if (written < size)
         queue_front (ctx, buffer + written, size - written);

      return size;
   }

   if ( (! (flags & lwp_stream_write_ignore_queue)) &&
         ( (ctx->flags & lwp_stream_flag_queueing)
           || list_length (ctx->back_queue) > 0))
   {
      lwp_trace ("%p : Adding to back queue (queueing = %d, front queue length = %d)",
            ctx, (int) ( (ctx->flags & lwp_stream_flag_queueing) != 0),
                            list_length (ctx->front_queue));

      if (flags & lwp_stream_write_partial)
         return 0;

      queue_back (ctx, buffer, size);

      if (ctx->retry == lw_stream_retry_more_data)
         lw_stream_retry (ctx, lw_stream_retry_now);

      return size;
   }
   
   /* If the stream def says the stream should be considered transparent, we
    * can skip sinking the data and just act as if it's already passed though.
    */

   if (ctx->def->is_transparent && ctx->def->is_transparent (ctx))
   {
      lw_stream_data (ctx, buffer, size);
      return size;
   }

   size_t written = ctx->def->sink_data ?
       ctx->def->sink_data (ctx, buffer, size) : size;

   if (flags & lwp_stream_write_partial)
      return written;

   if (written < size)
   {
      if (flags & lwp_stream_write_ignore_queue)
      {
         if (lwp_heapbuffer_length (&list_front (ctx->back_queue).buffer) == 0)
         {
            lwp_heapbuffer_add (&list_elem_front (ctx->back_queue)->buffer,
                                buffer + written, size - written);
         }
         else
         {
            /* TODO : rewind offset where possible instead of creating a new Queued? */

            struct _lwp_stream_queued queued = {};

            queued.type = lwp_stream_queued_data;

            lwp_heapbuffer_add (&queued.buffer, buffer + written, size - written);

            list_push_front (ctx->back_queue, queued);
         }
      }
      else
      {
         queue_back (ctx, buffer + written, size - written);
      }
   }

   return size;
}

void lw_stream_write_stream (lw_stream ctx, lw_stream source,
                             size_t size, lw_bool delete_when_finished)
{
   if (size == 0)
   {
      if (delete_when_finished)
         lw_stream_delete (source);

      return;
   }

   lwp_stream_write_stream (ctx, source, size, delete_when_finished ?
                              lwp_stream_write_delete_stream : 0);
}

void lwp_stream_write_stream (lw_stream ctx, lw_stream source,
                              size_t size, int flags)
{
   lw_bool should_queue = lw_false;

   if (! (flags & lwp_stream_write_ignore_queue))
   {
      if (list_length (ctx->back_queue) > 0
            || ctx->flags & lwp_stream_flag_queueing)
      {
         should_queue = lw_true;
      }
   }

   if (! (flags & lwp_stream_write_ignore_busy))
   {
      if (list_length (ctx->prev) > 0)
      {
         should_queue = lw_true;
      }
   }

   if (should_queue)
   {
      struct _lwp_stream_queued queued = {};

      queued.type = lwp_stream_queued_stream;
      queued.stream = source;
      queued.stream_bytes_left = size;
      queued.delete_stream = (flags & lwp_stream_write_delete_stream);

      list_push (ctx->back_queue, queued);

      return;
   }

   /* Are we currently in a different graph from the source stream? */

   if (ctx->graph != source->graph)
      lwp_streamgraph_swallow (source->graph, ctx->graph);

   assert (ctx->graph == source->graph);

   lwp_streamgraph_link link = (lwp_streamgraph_link) calloc (sizeof (*link), 1);
   
   link->from = source;
   link->to = ctx;
   link->bytes_left = size;
   link->delete_stream = (flags & lwp_stream_write_delete_stream);

   list_push (source->next, link);
   list_push (ctx->prev, link);

   /* This stream is now linked to, so doesn't need to be a root */

   list_remove (ctx->graph->roots, ctx);

   lwp_streamgraph_expand (ctx->graph);
   lwp_streamgraph_read (ctx->graph);
}

void lw_stream_write_file (lw_stream ctx, const char * filename)
{
   /* This method may only be used when the stream is associated with a pump */

   assert (ctx->pump);

   lw_file file = lw_file_new_open (ctx->pump, filename, "rb");

   if ( (!file) || !lw_fdstream_valid ((lw_fdstream) file))
      return;

   lw_stream_write_stream (ctx, (lw_stream) file,
                           lw_stream_bytes_left ((lw_stream) file), lw_true);
}

void lw_stream_add_filter_upstream (lw_stream ctx, lw_stream filter,
                                    lw_bool delete_with_stream,
                                    lw_bool close_together)
{
   lwp_stream_filterspec spec = (lwp_stream_filterspec) malloc (sizeof (*spec));

   spec->stream = ctx;
   spec->filter = filter;
   spec->delete_with_stream = delete_with_stream;
   spec->close_together = close_together;

   spec->link.bytes_left = -1;
   spec->link.delete_stream = lw_false;
   spec->link.to = 0;

   /* Upstream data passes through the most recently added filter first */

   list_push (ctx->filters_upstream, spec);
   list_push (filter->filtering, spec);

   if (filter->graph != ctx->graph)
      lwp_streamgraph_swallow (ctx->graph, filter->graph);

   lwp_streamgraph_expand (ctx->graph);
   lwp_streamgraph_read (ctx->graph);
}

void lw_stream_add_filter_downstream (lw_stream ctx, lw_stream filter,
                                      lw_bool delete_with_stream,
                                      lw_bool close_together)
{
   lwp_stream_filterspec spec = (lwp_stream_filterspec) malloc (sizeof (*spec));

   spec->stream = ctx;
   spec->filter = filter;
   spec->delete_with_stream = delete_with_stream;
   spec->close_together = close_together;

   spec->link.bytes_left = -1;
   spec->link.delete_stream = lw_false;
   spec->link.to = 0;

   /* Downstream data passes through the most recently added filter last */

   list_push (ctx->filters_downstream, spec);
   list_push (filter->filtering, spec);

   if (filter->graph != ctx->graph)
      lwp_streamgraph_swallow (ctx->graph, filter->graph);

   lwp_streamgraph_expand (ctx->graph);
   lwp_streamgraph_read (ctx->graph);
}

size_t lw_stream_bytes_left (lw_stream ctx)
{
   return ctx->def->bytes_left ? ctx->def->bytes_left (ctx) : -1;
}

void lw_stream_read (lw_stream ctx, size_t bytes)
{
   if ( (!ctx->def->read) || bytes == 0)
      return;

   ctx->def->read (ctx, bytes);
}

void lw_stream_data (lw_stream ctx, const char * buffer, size_t size)
{
   int num_data_hooks = list_length (ctx->exp_data_hooks);

   lwp_retain (ctx, "lw_stream_data");

   /* TODO: The data hook list would be faster to make a copy of if it was     
    * a real array.
    */

   lwp_stream_data_hook data_hooks = (lwp_stream_data_hook) alloca
       (sizeof (struct _lwp_stream_data_hook) * num_data_hooks);

   int i = 0;

   list_each (ctx->exp_data_hooks, hook)
   {
      data_hooks [i ++] = *hook;

      lwp_retain (hook->stream, "stream_data hook");
   }

   for (i = 0; i < num_data_hooks; ++ i)
   {
      lwp_stream_data_hook hook = &data_hooks [i];

      if (! (hook->stream->flags & lwp_stream_flag_dead))
         hook->proc (hook->stream, hook->tag, buffer, size);

      lwp_release (hook->stream, "stream_data hook");
   }

   /* Write the data to any streams next in the (expanded) graph, if this
    * stream still exists.
    */

   if (! (ctx->flags & lwp_stream_flag_dead))
   {
      lwp_stream_push (ctx, buffer, size);
   }

   lwp_release (ctx, "lw_stream_data");
}

void lwp_stream_push (lw_stream ctx, const char * buffer, size_t size)
{
   int num_links = list_length (ctx->next_expanded);

   if (!num_links)
      return;  /* nothing to do */

   lwp_retain (ctx, "stream_push");

   lwp_streamgraph_link * links = (lwp_streamgraph_link *) alloca
       (sizeof (lwp_streamgraph_link) * num_links);

   /* Copy the link dest pointers into our local array.
    *
    * TODO: This would be faster if the links were a real array in the first
    * place.
    */

   int i = 0;

   lwp_streamgraph_link link;

   list_each (ctx->next_expanded, link)
   {
      links [i ++] = link;
   }

   int last_expand = ctx->graph->last_expand;

   for (int i = 0; i < num_links; ++ i)
   {
      link = links [i];

      if (!link)
      {
         lwp_trace ("Link %d in the local Push list is now dead; skipping", i);
         continue;
      }

      lwp_trace ("Pushing to link %d of %d (%p)", i, num_links, link);

      size_t to_write = link->bytes_left != -1 && size > link->bytes_left ?
                           link->bytes_left : size;

      if (!lwp_stream_is_transparent (link->to_exp))
      {
         /* If we have some actual data, write it to the target stream */

         lwp_trace ("Link target is not transparent; buffer = %p", buffer);

         if (buffer)
         {
            lwp_stream_write (link->to_exp, buffer, to_write,
                  lwp_stream_write_ignore_filters |
                  lwp_stream_write_ignore_busy);
         }
      }
      else
      {
         lwp_trace ("Link target is transparent; pushing data forward");

         /* Target stream is transparent - have it push the data forward */

         lwp_stream_push (link->to_exp, buffer, to_write);
      }

      /* Pushing data may have caused this stream to be deleted */

      if (ctx->flags & lwp_stream_flag_dead)
         break;

      if (ctx->graph->last_expand != last_expand)
      {
         lwp_trace ("Graph has re-expanded - checking local Push list...");

         /* If any new links have been added to next_expanded, this push will
          * not write to them.  However, if any links in our local array
          * (yet to be written to) have disappeared, they must be removed.
          */

         for (int x = i; x < num_links; ++ x)
         {
            if (!list_find (ctx->next_expanded, links [x]))
            {
               if (link == links [x])
                  link = 0;

               links [x] = 0;
            }
         }

         if (!link)
            continue;
      }

      if (!link->to)
         continue; /* filters cannot expire */

      if (link->bytes_left == -1)
      {
         if (lw_stream_bytes_left (link->from_exp) != 0)
            continue;
      }
      else
      {
         if ((link->bytes_left -= to_write) > 0)
            continue;
      }

      if (link->delete_stream)
      {
         lw_stream_delete (link->from); /* will re-expand the graph */
      }
      else
      {
         lwp_streamgraph_clear_expanded (ctx->graph);

         list_remove (ctx->next, link);
         list_remove (link->to->prev, link);

         /* Since the target and anything after it are still part
          * of this graph, make it a root before deleting the link.
          */

         list_push (ctx->graph->roots, link->to);

         lwp_streamgraph_expand (ctx->graph);

         free (link);

         lwp_streamgraph_read (ctx->graph);
      }

      /* Maybe deleting the source stream caused this stream to be
       * deleted, too?
       */

      if (ctx->flags & lwp_stream_flag_dead)
         break;

      lwp_trace ("Graph re-expanded by Push - checking local list...");

      /* Since the graph has re-expanded, check the rest of the links we
       * intend to write to are still present in the list.
       */

      for (int x = i; x < num_links; ++ x)
         if (!list_find (ctx->next_expanded, links [x]))
            links [x] = 0;
   }

   if (lwp_release (ctx, "stream_push") || ctx->flags & lwp_stream_flag_dead)
      return;

   if (ctx->flags & lwp_stream_flag_closeASAP
         && lwp_stream_may_close (ctx))
   {
      lw_stream_close (ctx, lw_true);
   }
}

list_type (struct _lwp_stream_queued) lwp_stream_write_queue
    (lw_stream ctx, list (struct _lwp_stream_queued, queue))
{
   lwp_trace ("%p : WriteQueued : %d to write", ctx, list_length (queue));

   while (list_length (queue) > 0)
   {
      lwp_stream_queued queued = list_elem_front (queue);

      if (queued->type == lwp_stream_queued_begin_marker)
      {
         list_elem_remove (queued);
         ctx->flags |= lwp_stream_flag_queueing;

         break;
      }

      if (queued->type == lwp_stream_queued_data)
      {
         if (lwp_heapbuffer_length (&queued->buffer) > 0)
         {
            /* There's still something in the buffer that needs to be written */

            size_t written = lwp_stream_write
               ( ctx,
                 lwp_heapbuffer_buffer (&queued->buffer),
                 lwp_heapbuffer_length (&queued->buffer),
                 lwp_stream_write_ignore_queue | lwp_stream_write_partial
                      | lwp_stream_write_ignore_busy
               );

            lwp_heapbuffer_trim_left (&queued->buffer, written);

            if (lwp_heapbuffer_length (&queued->buffer) > 0)
               break; /* couldn't write everything */

            lwp_heapbuffer_free (&queued->buffer);
         }

         list_elem_remove (queued);
         continue;
      }

      if (queued->type == lwp_stream_queued_stream)
      {
         lw_stream stream = queued->stream;
         size_t bytes = queued->stream_bytes_left;

         int flags = lwp_stream_write_ignore_queue;

         if (queued->delete_stream)
            flags |= lwp_stream_write_delete_stream;

         list_elem_remove (queued);

         lwp_stream_write_stream (ctx, stream, bytes, flags);

         continue;
      }
   }

   return queue;
}

void lwp_stream_write_queued (lw_stream ctx)
{
   if (ctx->flags & lwp_stream_flag_queueing)
      return;

   if (ctx->flags & lwp_stream_flag_draining_queues)
      return;

   ctx->flags |= lwp_stream_flag_draining_queues;

   lwp_trace ("%p : Writing front queue (size = %d)",
               ctx, list_length (ctx->front_queue));

   lwp_retain (ctx, "write front queue");

   ctx->front_queue = lwp_stream_write_queue (ctx, ctx->front_queue); 

   if (lwp_release (ctx, "write front queue") || ctx->flags & lwp_stream_flag_dead)
      return

   lwp_trace ("%p : Front queue size is now %d, %d prev, %d in back queue",
         ctx, list_length (ctx->front_queue), list_length (ctx->prev),
                list_length (ctx->back_queue));

   if (list_length (ctx->front_queue) == 0
         && list_length (ctx->prev) == 0)
   {
      lwp_retain (ctx, "write back queue");

      ctx->back_queue = lwp_stream_write_queue (ctx, ctx->back_queue);

      if (lwp_release (ctx, "write back queue") || ctx->flags & lwp_stream_flag_dead)
         return;
   }

   if (ctx->flags & lwp_stream_flag_closeASAP
         && lwp_stream_may_close (ctx))
   {
      lw_stream_close (ctx, lw_true);
   }

   ctx->flags &= ~ lwp_stream_flag_draining_queues;
}

void lw_stream_retry (lw_stream ctx, int when)
{
   lwp_trace ("stream_retry for %p (prev_direct %p)", ctx, ctx->prev_direct);

   if (when == lw_stream_retry_now)
   {
      if (!lwp_stream_write_direct (ctx))
      {
         /* TODO: ??? */
      }

      lwp_stream_write_queued (ctx);
      return;
   }

   ctx->retry = when;
}

lw_bool lwp_stream_write_direct (lw_stream ctx)
{
   if (!ctx->prev_direct)
      return lw_true;

   if (!ctx->def->sink_stream)
      return lw_false;

   size_t written = ctx->def->sink_stream (ctx, ctx->prev_direct,
                                           ctx->direct_bytes_left);

   if (written != -1)
   {
      /* Pushing with a buffer of 0 pushes without any data (so the stream
       * logic can operate even though the data was already transmitted).
       */

      lwp_stream_push (ctx->prev_direct, 0, written);

      if (ctx->direct_bytes_left != -1)
         ctx->direct_bytes_left -= written;

      return lw_true;
   }

   return lw_false;
}

lw_bool lwp_stream_may_close (lw_stream ctx)
{
   return list_length (ctx->prev) == 0 &&
          list_length (ctx->back_queue) == 0 &&
          list_length (ctx->front_queue) == 0;
}

lw_bool lw_stream_close (lw_stream ctx, lw_bool immediate)
{
   if (ctx->flags & lwp_stream_flag_closing)
      return lw_false;

   if ( (!immediate) && !lwp_stream_may_close (ctx))
   {
      ctx->flags |= lwp_stream_flag_closeASAP;
      return lw_false;
   }

   if (ctx->def->close && !ctx->def->close (ctx, immediate))
   {
      assert (!immediate);

      /* The stream itself is ready to close, but something higher up isn't.
       * lw_stream_close should be called again later with immediate = true
       */

      return lw_false; 
   }

   ctx->flags |= lwp_stream_flag_closing;

   lwp_retain (ctx, "stream_close");

   /* If roots_expanded is already empty, something else has already cleared
    * the expanded graph (e.g. another stream closing) and should re-expand
    * later (meaning we don't have to bother)
    */

   lw_bool already_cleared = list_length (ctx->graph->roots_expanded) == 0;

   if (!already_cleared)
      lwp_streamgraph_clear_expanded (ctx->graph);


   /* Anything that comes before us can no longer link here */

   list_each (ctx->prev, link)
   {
      list_remove (link->from->next, link);

      free (link);
   }

   list_clear (ctx->prev);


   /* Anything that comes after us will have to be a root */

   list_each (ctx->next, link)
   {
      list_push (ctx->graph->roots, link->to);
      list_remove (link->to->prev, link);

      free (link);
   }

   list_clear (ctx->next);


   /* If we're set to close together with any filters, close those too (this
    * is the reason for the already_cleared check)
    */

   /* TODO: For non-immediate close, don't call this stream's close
    * hooks until all filters have finished closing.
    */

   lw_stream * to_close  = (lw_stream *) alloca (sizeof (lw_stream) *
                              (list_length (ctx->filtering)
                                  + list_length (ctx->filters_upstream)
                                  + list_length (ctx->filters_downstream)));

   int n = 0;

   list_each (ctx->filtering, spec)
   {
      if (spec->close_together)
      {
         lwp_retain (spec->stream, "stream_close filter");
         to_close [n ++] = spec->stream;
      }
   }

   list_each (ctx->filters_upstream, spec)
   {
      if (spec->close_together)
      {
         lwp_retain (spec->filter, "stream_close filter");
         to_close [n ++] = spec->filter;
      }
   }

   list_each (ctx->filters_downstream, spec)
   {
      if (spec->close_together)
      {
         lwp_retain (spec->filter, "stream_close filter");
         to_close [n ++] = spec->filter;
      }
   }

   for (int i = 0; i < n; ++ i)
   {
      lw_stream stream = to_close [i];

      /* TODO: see above wrt immediate = true */
      lw_stream_close (stream, lw_true);

      lwp_release (stream, "stream_close filter");
   }

   if (!already_cleared)
   {
      if (!list_find (ctx->graph->roots, ctx))
         list_push (ctx->graph->roots, ctx);

      lwp_streamgraph_expand (ctx->graph);
      lwp_streamgraph_read (ctx->graph);
   }

   list_each (ctx->close_hooks, hook)
   {
      hook.proc (ctx, hook.tag);

      if (ctx->flags & lwp_stream_flag_dead)
         break;  /* close hook destroyed the stream */
   }

   ctx->flags &= ~ lwp_stream_flag_closing;

   lwp_release (ctx, "stream_close");

   return lw_true;
}

void lw_stream_begin_queue (lw_stream stream)
{
   if (list_length (stream->back_queue)
         || list_length (stream->back_queue))
   {
      /* Although we're going to start queueing any new data, whatever is
       * currently in the queue still needs to be written.  A queued item with
       * the begin_marker type indicates where the stream should stop writing
       * and set the queueing flag.
       */

      struct _lwp_stream_queued queued = {};

      queued.type = lwp_stream_queued_begin_marker;

      list_push (stream->back_queue, queued);
   }
   else
   {
      stream->flags |= lwp_stream_flag_queueing;
   }
}

size_t lw_stream_queued (lw_stream stream)
{
   size_t size = 0, bytes_left;

   list_each (stream->back_queue, queued)
   {
      if (queued.type == lwp_stream_queued_data)
      {
         size += lwp_heapbuffer_length (&queued.buffer);
         continue;
      }

      if (queued.type == lwp_stream_queued_stream)
      {
         if (!queued.stream)
            continue;

         if (queued.stream_bytes_left != -1)
         {
            size += queued.stream_bytes_left;
            continue;
         }

         if ((bytes_left = lw_stream_bytes_left (queued.stream)) == -1)
            return -1;

         size += bytes_left;

         continue;
      }
   }

   return size;
}

void lw_stream_end_queue_hb (lw_stream ctx, int num_head_buffers,
                             const char ** buffers, size_t * lengths)
{
   for (int i = 0; i < num_head_buffers; ++ i)
   {
      lwp_stream_write (ctx, buffers [i], lengths [i],
            lwp_stream_write_ignore_queue | lwp_stream_write_ignore_busy);
   }

   lw_stream_end_queue (ctx);
}

void lw_stream_end_queue (lw_stream ctx)
{
   lwp_trace ("%p : end_queue called", ctx);

   /* TODO : Look for a queued item w/ Flag_BeginQueue if Queueing is false? */

   assert (ctx->flags & lwp_stream_flag_queueing);

   ctx->flags &= ~ lwp_stream_flag_queueing;

   lwp_stream_write_queued (ctx);
}

lw_bool lwp_stream_is_transparent (lw_stream ctx)
{
   assert (! (ctx->flags & lwp_stream_flag_dead));

   if (list_length (ctx->exp_data_hooks) > 0)
      return lw_false;

   if (list_length (ctx->back_queue) > 0
         || list_length (ctx->front_queue) > 0)
   {
      return lw_false;
   }

   if (ctx->flags & lwp_stream_flag_queueing)
      return lw_false;

   return ctx->def->is_transparent && ctx->def->is_transparent (ctx);
}

void lw_stream_add_hook_data (lw_stream stream,
                              lw_stream_hook_data proc,
                              void * tag)
{   
   /* TODO : Prevent the same hook being registered twice? */

   lwp_stream_data_hook hook = (lwp_stream_data_hook) calloc (sizeof (*hook), 1);

   hook->proc = proc;
   hook->stream = stream;
   hook->tag = tag;

   list_push (stream->data_hooks, hook);

   lwp_streamgraph_clear_expanded (stream->graph);
   lwp_streamgraph_expand (stream->graph);

   /* TODO: Do we need to call lwp_streamgraph_read here? */
} 

void lw_stream_remove_hook_data (lw_stream stream,
                                    lw_stream_hook_data proc,
                                    void * tag)
{   
   list_each_elem (stream->data_hooks, hook)
   {
      if ((*hook)->proc == proc && (*hook)->tag == tag)
      {
         list_elem_remove (hook);
         break;
      }
   }

   lwp_streamgraph_clear_expanded (stream->graph);
   lwp_streamgraph_expand (stream->graph);
}

void lw_stream_add_hook_close (lw_stream stream,
                               lw_stream_hook_close proc,
                               void * tag)
{   
   struct _lwp_stream_close_hook hook = { proc, tag };
   list_push (stream->close_hooks, hook);

   lwp_streamgraph_clear_expanded (stream->graph);
   lwp_streamgraph_expand (stream->graph);
} 

void lw_stream_remove_hook_close (lw_stream stream,
                                  lw_stream_hook_close proc,
                                  void * tag)
{   

   list_each_elem (stream->close_hooks, hook)
   {
      if (hook->proc == proc && hook->tag == tag)
      {
         list_elem_remove (hook);
         break;
      }
   }

   lwp_streamgraph_clear_expanded (stream->graph);
   lwp_streamgraph_expand (stream->graph);
}


void lw_stream_set_tag (lw_stream ctx, void * tag)
{
   ctx->tag = tag;
}

void * lw_stream_tag (lw_stream ctx)
{
   return ctx->tag;
}

lw_pump lw_stream_pump (lw_stream ctx)
{
   return ctx->pump;
}

