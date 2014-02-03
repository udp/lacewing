
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

#ifndef _lw_refcount_h
#define _lw_refcount_h

#define MAX_REFS  32

struct lwp_refcount
{
   unsigned short refcount;           
   void (* on_dealloc) (void *);

   char name [64];
   const char * refs[MAX_REFS];

   lw_bool enable_logging;
};

lw_bool _lwp_retain (struct lwp_refcount * refcount, const char * name);
lw_bool _lwp_release (struct lwp_refcount * refcount, const char * name);

#define lwp_refcounted                                                        \
struct lwp_refcount refcount;                                              \

#define lwp_retain(x, name)                                                   \
   _lwp_retain ((struct lwp_refcount *) (x), name)                            \

#define lwp_release(x, name)                                                  \
   _lwp_release ((struct lwp_refcount *) (x), name)                           \

#define lwp_set_dealloc_proc(x, proc)                                         \
   *(void **)&(((struct lwp_refcount *) (x))->on_dealloc) = (void *)(proc);  \

#define lwp_set_refcount_name(x, n)                                           \
   strcpy (((struct lwp_refcount *) (x))->name, (n));                         \

#define lwp_enable_refcount_logging(x, name)                                  \
   lwp_set_refcount_name (x, name);                                           \
   ((struct lwp_refcount *) (x))->enable_logging = lw_true;                   \

#endif
   

