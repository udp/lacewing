
/* vim: set et ts=3 sw=3 ft=cpp:
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

#include "../common.h"

void lacewing::pump_delete (lacewing::pump pump)
{
   lw_pump_delete ((lw_pump) pump);
}

void _pump::add_user ()
{
   lw_pump_add_user ((lw_pump) this);
}

void _pump::remove_user ()
{
   lw_pump_remove_user ((lw_pump) this);
}

bool _pump::in_use ()
{
   return lw_pump_in_use ((lw_pump) this);
}

#ifdef _WIN32

 lw_pump_watch _pump::add (HANDLE handle, void * tag,
                           lw_pump_callback callback)
 {
    return lw_pump_add ((lw_pump) this, handle, tag, callback);
 }

 void _pump::update_callbacks (lw_pump_watch watch, void * tag,
                               lw_pump_callback callback)
 {
    lw_pump_update_callbacks ((lw_pump) this, watch, tag, callback);
 }

#else

 lw_pump_watch _pump::add (int fd, void * tag,
                           lw_pump_callback on_read_ready,
                           lw_pump_callback on_write_ready,
                           bool edge_triggered)
 {
    return (lw_pump_watch) lw_pump_add
       ((lw_pump) this, fd, tag, on_read_ready, on_write_ready, edge_triggered);
 }

 void _pump::update_callbacks (lw_pump_watch watch, void * tag,
                               lw_pump_callback on_read_ready,
                               lw_pump_callback on_write_ready,
                               bool edge_triggered)
 {
    lw_pump_update_callbacks ((lw_pump) this, watch, tag, on_read_ready,
                              on_write_ready, edge_triggered);
 }

#endif
 
void _pump::remove (lw_pump_watch watch)
{
   lw_pump_remove ((lw_pump) this, watch);
}

void _pump::post (void * func, void * param)
{
   lw_pump_post ((lw_pump) this, func, param);
}

void * _pump::tag ()
{
   return lw_pump_tag ((lw_pump) this);
}

void _pump::tag (void * tag)
{
   lw_pump_set_tag ((lw_pump) this, tag);
}

