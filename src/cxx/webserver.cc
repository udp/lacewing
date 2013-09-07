
/* vim: set et ts=3 sw=3 ft=cpp:
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

#include "../common.h"

webserver lacewing::webserver_new (lacewing::pump pump)
{
   return (webserver) lw_ws_new ((lw_pump) pump);
}

void lacewing::webserver_delete (lacewing::webserver webserver)
{
   lw_ws_delete ((lw_ws) webserver);
}

void _webserver::host (long port)
{
   lw_ws_host ((lw_ws) this, port);
}

void _webserver::host_secure (long port)
{
   lw_ws_host_secure ((lw_ws) this, port);
}

void _webserver::host (lacewing::filter filter)
{
   lw_ws_host_filter ((lw_ws) this, (lw_filter) filter);
}

void _webserver::host_secure (lacewing::filter filter)
{
   lw_ws_host_secure_filter ((lw_ws) this, (lw_filter) filter);
}

void _webserver::unhost ()
{
   lw_ws_unhost ((lw_ws) this);
}

void _webserver::unhost_secure ()
{
   lw_ws_unhost_secure ((lw_ws) this);
}

bool _webserver::hosting ()
{
   return lw_ws_hosting ((lw_ws) this);
}

bool _webserver::hosting_secure ()
{
   return lw_ws_hosting_secure ((lw_ws) this);
}

long _webserver::port ()
{
   return lw_ws_port ((lw_ws) this);
}

long _webserver::port_secure ()
{
   return lw_ws_port_secure ((lw_ws) this);
}

bool _webserver::load_cert_file (const char * filename, const char * passphrase)
{
   return lw_ws_load_cert_file ((lw_ws) this, filename, passphrase);
}

bool _webserver::load_sys_cert (const char * store_name, const char * common_name,
                                const char * location)
{
   return lw_ws_load_sys_cert ((lw_ws) this, store_name, common_name, location);
}

bool _webserver::cert_loaded ()
{
   return lw_ws_cert_loaded ((lw_ws) this);
}

void _webserver::enable_manual_finish ()
{
   lw_ws_enable_manual_finish ((lw_ws) this);
}

long _webserver::idle_timeout ()
{
   return lw_ws_idle_timeout ((lw_ws) this);
}

void _webserver::idle_timeout (long sec)
{
   lw_ws_set_idle_timeout ((lw_ws) this, sec);
}

void _webserver::session_close (const char * id)
{
   lw_ws_session_close ((lw_ws) this, id);
}

void _webserver::on_get (_webserver::hook_get hook)
{
   lw_ws_on_get ((lw_ws) this, (lw_ws_hook_get) hook);
}

void _webserver::on_post (_webserver::hook_post hook)
{
   lw_ws_on_post ((lw_ws) this, (lw_ws_hook_post) hook);
}

void _webserver::on_head (_webserver::hook_head hook)
{
   lw_ws_on_head ((lw_ws) this, (lw_ws_hook_head) hook);
}

void _webserver::on_disconnect (_webserver::hook_disconnect hook)
{
   lw_ws_on_disconnect ((lw_ws) this, (lw_ws_hook_disconnect) hook);
}

void _webserver::on_error (_webserver::hook_error hook)
{
   lw_ws_on_error ((lw_ws) this, (lw_ws_hook_error) hook);
}

void _webserver::on_upload_start (_webserver::hook_upload_start hook)
{
   lw_ws_on_upload_start ((lw_ws) this, (lw_ws_hook_upload_start) hook);
}

void _webserver::on_upload_chunk (_webserver::hook_upload_chunk hook)
{
   lw_ws_on_upload_chunk ((lw_ws) this, (lw_ws_hook_upload_chunk) hook);
}

void _webserver::on_upload_done (_webserver::hook_upload_done hook)
{
   lw_ws_on_upload_done ((lw_ws) this, (lw_ws_hook_upload_done) hook);
}

void _webserver::on_upload_post (_webserver::hook_upload_post hook)
{
   lw_ws_on_upload_post ((lw_ws) this, (lw_ws_hook_upload_post) hook);
}

lacewing::address _webserver_request::address ()
{
   return (lacewing::address) lw_ws_req_addr ((lw_ws_req) this);
}

bool _webserver_request::secure ()
{
   return lw_ws_req_secure ((lw_ws_req) this);
}

const char * _webserver_request::url ()
{
   return lw_ws_req_url ((lw_ws_req) this);
}

const char * _webserver_request::hostname ()
{
   return lw_ws_req_hostname ((lw_ws_req) this);
}

void _webserver_request::disconnect ()
{
   lw_ws_req_disconnect ((lw_ws_req) this);
}

void _webserver_request::redirect (const char * url)
{
   lw_ws_req_set_redirect ((lw_ws_req) this, url);
}

void _webserver_request::status (long code, const char * message)
{
   lw_ws_req_status ((lw_ws_req) this, code, message);
}

void _webserver_request::set_mimetype (const char * mimetype,
                                       const char * charset)
{
   lw_ws_req_set_mimetype_ex ((lw_ws_req) this, mimetype, charset);
}

void _webserver_request::guess_mimetype (const char * filename)
{
   lw_ws_req_guess_mimetype ((lw_ws_req) this, filename);
}

void _webserver_request::finish ()
{
   lw_ws_req_finish ((lw_ws_req) this);
}

long _webserver_request::idle_timeout ()
{
   return lw_ws_req_idle_timeout ((lw_ws_req) this);
}

void _webserver_request::idle_timeout (long sec)
{
   lw_ws_req_set_idle_timeout ((lw_ws_req) this, sec);
}

lw_i64 _webserver_request::last_modified ()
{
   return lw_ws_req_last_modified ((lw_ws_req) this);
}

void _webserver_request::last_modified (lw_i64 time)
{
   lw_ws_req_set_last_modified ((lw_ws_req) this, time);
}

void _webserver_request::set_unmodified ()
{
   lw_ws_req_set_unmodified ((lw_ws_req) this);
}

void _webserver_request::disable_cache ()
{
   lw_ws_req_disable_cache ((lw_ws_req) this);
}

void _webserver_request::enable_download_resuming ()
{
   /* lw_ws_req_enable_download_resuming ((lw_ws_req) this); */
}

webserver_request_header _webserver_request::header_first ()
{
   return (webserver_request_header) lw_ws_req_hdr_first ((lw_ws_req) this);
}

webserver_request_header _webserver_request_header::next ()
{
   return (webserver_request_header) lw_ws_req_hdr_next ((lw_ws_req_hdr) this);
}

const char * _webserver_request_header::name ()
{
   return lw_ws_req_hdr_name ((lw_ws_req_hdr) this);
}

const char * _webserver_request_header::value ()
{
   return lw_ws_req_hdr_value ((lw_ws_req_hdr) this);
}

const char * _webserver_request::header (const char * name)
{
   return lw_ws_req_header ((lw_ws_req) this, name);
}

void _webserver_request::header (const char * name, const char * value)
{
   lw_ws_req_set_header ((lw_ws_req) this, name, value);
}

void _webserver_request::add_header (const char * name, const char * value)
{
   lw_ws_req_add_header ((lw_ws_req) this, name, value);
}

webserver_request_cookie _webserver_request::cookie_first ()
{
   return (webserver_request_cookie) lw_ws_req_cookie_first ((lw_ws_req) this);
}

webserver_request_cookie _webserver_request_cookie::next ()
{
   return (webserver_request_cookie) lw_ws_req_cookie_next ((lw_ws_req_cookie) this);
}

const char * _webserver_request_cookie::name ()
{
   return lw_ws_req_cookie_name ((lw_ws_req_cookie) this);
}

const char * _webserver_request_cookie::value ()
{
   return lw_ws_req_cookie_value ((lw_ws_req_cookie) this);
}

const char * _webserver_request::cookie (const char * name)
{
   return lw_ws_req_get_cookie ((lw_ws_req) this, name);
}

void _webserver_request::cookie (const char * name, const char * value)
{
   lw_ws_req_set_cookie ((lw_ws_req) this, name, value);
}

void _webserver_request::cookie (const char * name, const char * value, const char * attr)
{
   lw_ws_req_set_cookie_attr ((lw_ws_req) this, name, value, attr);
}

webserver_sessionitem _webserver_request::session_first ()
{
   return (webserver_sessionitem) lw_ws_req_session_first ((lw_ws_req) this);
}

webserver_sessionitem _webserver_sessionitem::next ()
{
   return (webserver_sessionitem) lw_ws_sessionitem_next ((lw_ws_sessionitem) this);
}

const char * _webserver_sessionitem::name ()
{
   return lw_ws_sessionitem_name ((lw_ws_sessionitem) this);
}

const char * _webserver_sessionitem::value ()
{
   return lw_ws_sessionitem_value ((lw_ws_sessionitem) this);
}

const char * _webserver_request::session (const char * name)
{
   return lw_ws_req_session_read ((lw_ws_req) this, name);
}

void _webserver_request::session (const char * name, const char * value)
{
   lw_ws_req_session_write ((lw_ws_req) this, name, value);
}

void _webserver_request::session_close ()
{
   lw_ws_req_session_close ((lw_ws_req) this);
}

webserver_request_param _webserver_request::GET ()
{
   return (webserver_request_param) lw_ws_req_GET_first ((lw_ws_req) this);
}

webserver_request_param _webserver_request::POST ()
{
   return (webserver_request_param) lw_ws_req_POST_first ((lw_ws_req) this);
}

webserver_request_param _webserver_request_param::next ()
{
   return (webserver_request_param) lw_ws_req_param_next ((lw_ws_req_param) this);
}

const char * _webserver_request_param::name ()
{
   return lw_ws_req_param_name ((lw_ws_req_param) this);
}

const char * _webserver_request_param::value ()
{
   return lw_ws_req_param_value ((lw_ws_req_param) this);
}

const char * _webserver_request::GET (const char * name)
{
   return lw_ws_req_GET ((lw_ws_req) this, name);
}

const char * _webserver_request::POST (const char * name)
{
   return lw_ws_req_POST ((lw_ws_req) this, name);
}

const char * _webserver_request::body ()
{
   return lw_ws_req_body ((lw_ws_req) this);
}

const char * _webserver_upload::form_element_name ()
{
   return lw_ws_upload_form_el_name ((lw_ws_upload) this);
}

const char * _webserver_upload::filename ()
{
   return lw_ws_upload_filename ((lw_ws_upload) this);
}

void _webserver_upload::set_autosave ()
{
   lw_ws_upload_set_autosave ((lw_ws_upload) this);
}

const char * _webserver_upload::autosave_filename ()
{
   return lw_ws_upload_autosave_fname ((lw_ws_upload) this);
}

const char * _webserver_upload::header (const char * name)
{
   return lw_ws_upload_header ((lw_ws_upload) this, name);
}

webserver_upload_header _webserver_upload::header_first ()
{
   return (webserver_upload_header) lw_ws_upload_hdr_first ((lw_ws_upload) this);
}

webserver_upload_header _webserver_upload_header::next ()
{
   return (webserver_upload_header) lw_ws_upload_hdr_next ((lw_ws_upload_hdr) this);
}

const char * _webserver_upload_header::name ()
{
   return lw_ws_upload_hdr_name ((lw_ws_upload_hdr) this);
}

const char * _webserver_upload_header::value ()
{
   return lw_ws_upload_hdr_value ((lw_ws_upload_hdr) this);
}

void * _webserver::tag ()
{
   return lw_ws_tag ((lw_ws) this);
}

void _webserver::tag (void * tag)
{
   lw_ws_set_tag ((lw_ws) this, tag);
}

