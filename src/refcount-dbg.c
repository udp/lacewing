
/* vim: set et ts=3 sw=3 sts=3 ft=c:
 *
 * Copyright (C) 2014 James McLaughlin.  All rights reserved.
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

#ifdef _lacewing_debug

static void list_refs (char * buf, struct lwp_refcount * refcount)
{
   *buf = 0;

   for (int i = 0; i < refcount->refcount; ++ i)
   {
       if (!refcount->buf [i])
           continue;

       if (*buf)
           strcat (buf, ", ");

       strcat (buf, refcount->buf [i]);
   }
}

lw_bool _lwp_retain (struct lwp_refcount * refcount, const char * name)
{
   /* sanity check
    */
   assert (refcount->refcount < MAX_REFS);

   if (refcount->enable_logging)
   {
       char refs [1024];
       list_refs (refs, refcount);

       lw_trace ("refcount: %s @ %p has %d refs (%s), now retaining (%s)",
                    refcount->name,
                    refcount,
                    (int) refcount->refcount,
                    refs,
                    name);
   }

   for (int i = 0; i < MAX_REFS; ++ i)
   {
       if (!refcount->refs [i])
       {
           refcount->refs [i] = name;
           break;
       }
   }

   return lw_false;
}

lw_bool _lwp_release (struct lwp_refcount * refcount, const char * name)
{
   assert (refcount->refcount >= 1 && refcount->refcount < MAX_REFS);

   -- refcount->refcount;

   for (int i = 0; i < MAX_REFS; ++ i)
   {
       if (!strcasecmp (refcount->refs [i], name))
       {
           refcount->refs [i] = NULL;
           break;
       }
   }

   if (refcount->enable_logging)
   {
       char refs [1024];
       list_refs (refs, refcount);

       lw_trace ("refcount: %s @ %p released by %s, now has %d refs (%s)",
                    refcount->name,
                    refcount,
                    name,
                    (int) refcount->refcount,
                    refs);
   }

   if (refcount->refcount == 0)
   {
      if (refcount->on_dealloc)
         refcount->on_dealloc ((void *) refcount);
      else
         free (refcount);

      return lw_true;
   }

   return lw_false;
}

#endif

