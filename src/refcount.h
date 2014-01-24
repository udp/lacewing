
/* vim: set et ts=3 sw=3 ft=c:
 *
 * Copyright (C) 2013 James McLaughlin.  All rights reserved.
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

#ifndef _lw_refcount_h
#define _lw_refcount_h

struct lwp_refcount
{
   unsigned short refcount;           
   void (* on_dealloc) (void *);

   #ifdef _lacewing_debug
      void (* on_retain) (void *);
      void (* on_release) (void *);
      char name [64];
   #endif
};

static inline lw_bool _lwp_retain (struct lwp_refcount * refcount)
{
   /* sanity check
    */
   assert (refcount->refcount < 100);

   #ifdef _lacewing_debug
      if (refcount->on_retain)
         refcount->on_retain ((void *) refcount);
   #endif

   ++ refcount->refcount;

   return lw_false;
}

static inline lw_bool _lwp_release (struct lwp_refcount * refcount)
{
   assert (refcount->refcount >= 1 && refcount->refcount < 100);

   #ifdef _lacewing_debug
      if (refcount->on_release)
         refcount->on_release ((void *) refcount);
   #endif

   if ((-- refcount->refcount) == 0)
   {
      if (refcount->on_dealloc)
         refcount->on_dealloc ((void *) refcount);
      else
         free (refcount);

      return lw_true;
   }

   return lw_false;
}

#define lwp_refcounted                                                        \
   struct lwp_refcount refcount;                                              \

#define lwp_retain(x)                                                         \
   _lwp_retain ((struct lwp_refcount *) (x))                                  \

#define lwp_release(x)                                                        \
   _lwp_release ((struct lwp_refcount *) (x))                                 \

#define lwp_set_dealloc_proc(x, proc) do {                                    \
  *(void **) &(((struct lwp_refcount *) (x))->on_dealloc) = (void *) (proc);  \
} while (0);                                                                  \

#ifdef _lacewing_debug

   #define lwp_set_retain_proc(x, proc) do {                                     \
     *(void **) &(((struct lwp_refcount *) (x))->on_retain) = (void *) (proc);   \
   } while (0);                                                                  \

   #define lwp_set_release_proc(x, proc) do {                                    \
     *(void **) &(((struct lwp_refcount *) (x))->on_release) = (void *) (proc);  \
   } while (0);                                                                  \

   #define lwp_set_refcount_name(x, n) do {                                      \
     strcpy (((struct lwp_refcount *) (x))->name, n);                            \
   } while (0);                                                                  \

   void lwp_refcount_log_retain (struct lwp_refcount *);
   void lwp_refcount_log_release (struct lwp_refcount *);

   #define lwp_enable_refcount_logging(x, name) do {        \
      lwp_set_refcount_name (x, name);                      \
      lwp_set_retain_proc (x, lwp_refcount_log_retain);     \
      lwp_set_release_proc (x, lwp_refcount_log_release);   \
   } while (0);                                             \

#else

   #define lwp_set_retain_proc(x, proc)
   #define lwp_set_release_proc(x, proc)
   #define lwp_set_refcount_name(x, name)
   #define lwp_enable_refcount_logging(x, name)

#endif
   
#endif

