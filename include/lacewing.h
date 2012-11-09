
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

#ifndef LacewingIncluded
#define LacewingIncluded

#include <stdarg.h>
#include <stdio.h>

#ifndef _MSC_VER

    #ifndef __STDC_FORMAT_MACROS
        #define __STDC_FORMAT_MACROS
    #endif

    #include <inttypes.h>
    
    #define lw_i64   int64_t
    #define lw_ui64   uint64_t
    #define lw_iptr  intptr_t
    #define lw_i32   int32_t
    #define lw_i16   int16_t
    #define lw_i8    int8_t

    #define lw_PRId64 PRId64
    #define lw_PRIu64 PRIu64

#else

    #ifdef _WIN64
        #define lw_iptr __int64
    #else
        #define lw_iptr __int32
    #endif

    #define lw_i64  __int64
    #define lw_ui64  unsigned __int64
    #define lw_i32  __int32
    #define lw_i16  __int16
    #define lw_i8   __int8

    #define lw_PRId64 "I64d"
    #define lw_PRIu64 "I64u"
    
#endif

#ifndef _WIN32
    #ifndef LacewingHandler
        #define LacewingHandler
    #endif

    #ifndef LacewingFunction
        #define LacewingFunction
    #endif
#else

    /* For the definition of HANDLE and OVERLAPPED (for Pump) */

    #ifndef _INC_WINDOWS
        #include <winsock2.h>
        #include <windows.h>
    #endif

    #define LacewingHandler __cdecl

#endif

#ifndef LacewingFunction
    #define LacewingFunction __declspec (dllimport)
#endif

typedef lw_i8 lw_bool;

#define lw_true   ((lw_bool) 1)
#define lw_false  ((lw_bool) 0)

#ifdef _WIN32
    typedef HANDLE lw_fd;
#else
    typedef int lw_fd;
#endif

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#define LacewingFlat(c) \
    typedef struct { void * reserved; void * tag; } c

LacewingFunction    const char* lw_version                  ();
LacewingFunction        lw_i64  lw_file_last_modified       (const char * filename);
LacewingFunction       lw_bool  lw_file_exists              (const char * filename);
LacewingFunction        size_t  lw_file_size                (const char * filename);
LacewingFunction       lw_bool  lw_path_exists              (const char * filename);
LacewingFunction          void  lw_temp_path                (char * buffer);
LacewingFunction    const char* lw_guess_mime_type          (const char * filename);
LacewingFunction          void  lw_md5                      (char * output, const char * input, size_t length);
LacewingFunction          void  lw_md5_hex                  (char * output, const char * input, size_t length);
LacewingFunction          void  lw_sha1                     (char * output, const char * input, size_t length);
LacewingFunction          void  lw_sha1_hex                 (char * output, const char * input, size_t length);
LacewingFunction          void  lw_dump                     (const char * buffer, size_t size);
LacewingFunction       lw_bool  lw_random                   (char * buffer, size_t size);

/* Thread */

  LacewingFlat (lw_thread);

  LacewingFunction      lw_thread* lw_thread_new      (const char * name, void * function);
  LacewingFunction           void  lw_thread_delete   (lw_thread *);
  LacewingFunction           void  lw_thread_start    (lw_thread *, void * parameter);
  LacewingFunction        lw_bool  lw_thread_started  (lw_thread *);
  LacewingFunction           long  lw_thread_join     (lw_thread *);

/* Address */

  LacewingFlat (lw_addr);

  LacewingFunction        lw_addr* lw_addr_new          (const char * hostname, const char * service);
  LacewingFunction        lw_addr* lw_addr_new_port     (const char * hostname, long port);
  LacewingFunction        lw_addr* lw_addr_new_ex       (const char * hostname, const char * service, long hints);
  LacewingFunction        lw_addr* lw_addr_new_port_ex  (const char * hostname, long port, long hints);
  LacewingFunction        lw_addr* lw_addr_copy         (lw_addr *);
  LacewingFunction           void  lw_addr_delete       (lw_addr *);
  LacewingFunction           long  lw_addr_port         (lw_addr *);
  LacewingFunction           void  lw_addr_set_port     (lw_addr *, long port);
  LacewingFunction        lw_bool  lw_addr_is_ready     (lw_addr *);
  LacewingFunction        lw_bool  lw_addr_is_ipv6      (lw_addr *);
  LacewingFunction        lw_bool  lw_addr_is_equal     (lw_addr *, lw_addr *);
  LacewingFunction     const char* lw_addr_tostring     (lw_addr *);

/* Filter */

  LacewingFlat (lw_filter);

  LacewingFunction      lw_filter* lw_filter_new                ();
  LacewingFunction           void  lw_filter_delete             (lw_filter *);
  LacewingFunction      lw_filter* lw_filter_copy               (lw_filter *);
  LacewingFunction           void  lw_filter_set_remote         (lw_filter *, lw_addr *);
  LacewingFunction        lw_addr* lw_filter_get_remote         (lw_filter *);
  LacewingFunction           void  lw_filter_set_local          (lw_filter *, lw_addr *);
  LacewingFunction        lw_addr* lw_filter_get_local          (lw_filter *);
  LacewingFunction           void  lw_filter_set_local_port     (lw_filter *, long port);
  LacewingFunction           long  lw_filter_get_local_port     (lw_filter *);
  LacewingFunction           void  lw_filter_set_remote_port    (lw_filter *, long port);
  LacewingFunction           long  lw_filter_get_remote_port    (lw_filter *);
  LacewingFunction           void  lw_filter_set_reuse          (lw_filter *);
  LacewingFunction        lw_bool  lw_filter_is_reuse_set       (lw_filter *);
  LacewingFunction           void  lw_filter_set_ipv6           (lw_filter *, lw_bool);
  LacewingFunction        lw_bool  lw_filter_is_ipv6            (lw_filter *);

/* Pump */

  LacewingFlat (lw_pump);
  LacewingFlat (lw_pump_watch);

  LacewingFunction           void  lw_pump_delete               (lw_pump *);
  LacewingFunction           void  lw_pump_add_user             (lw_pump *);
  LacewingFunction           void  lw_pump_remove_user          (lw_pump *);
  LacewingFunction        lw_bool  lw_pump_in_use               (lw_pump *);
  LacewingFunction           void  lw_pump_remove               (lw_pump *, lw_pump_watch *);
  LacewingFunction           void  lw_pump_post                 (lw_pump *, void * fn, void * param);
  LacewingFunction        lw_bool  lw_pump_is_eventpump         (lw_pump *);

  #ifdef _WIN32

    typedef void (LacewingHandler * lw_pump_callback)
        (void * tag, OVERLAPPED *, unsigned int bytes, int error);

    LacewingFunction lw_pump_watch* lw_pump_add
                          (lw_pump *, HANDLE, void * tag, lw_pump_callback);

    LacewingFunction void lw_pump_update_callbacks
                          (lw_pump *, lw_pump_watch *, void * tag, lw_pump_callback);
  #else

    typedef void (LacewingHandler * lw_pump_callback) (void * tag);

    LacewingFunction lw_pump_watch* lw_pump_add
                          (lw_pump *, int FD, void * tag, lw_pump_callback onReadReady,
                              lw_pump_callback onWriteReady, lw_bool edge_triggered);

    LacewingFunction void lw_pump_update_callbacks
                          (lw_pump *, lw_pump_watch *, void * tag, lw_pump_callback onReadReady,
                              lw_pump_callback onWriteReady, lw_bool edge_triggered);
  #endif

/* EventPump */

  LacewingFunction        lw_pump* lw_eventpump_new                  ();
  LacewingFunction           void  lw_eventpump_tick                 (lw_pump *);
  LacewingFunction           void  lw_eventpump_start_event_loop     (lw_pump *);
  LacewingFunction           void  lw_eventpump_start_sleepy_ticking (lw_pump *, void (LacewingHandler * on_tick_needed) (lw_pump *));
  LacewingFunction           void  lw_eventpump_post_eventloop_exit  (lw_pump *);

/* Stream */

  LacewingFlat (lw_stream);

  LacewingFunction   lw_bool  lw_stream_valid                 (lw_stream *);
  LacewingFunction    size_t  lw_stream_bytes_left            (lw_stream *);
  LacewingFunction      void  lw_stream_read                  (lw_stream *, size_t bytes);
  LacewingFunction      void  lw_stream_begin_queue           (lw_stream *);
  LacewingFunction    size_t  lw_stream_queued                (lw_stream *);
  LacewingFunction      void  lw_stream_end_queue             (lw_stream *, int head_buffers, const char ** buffers, size_t * lengths);
  LacewingFunction      void  lw_stream_clear_queue           (lw_stream *);
  LacewingFunction      void  lw_stream_write                 (lw_stream *, const char * buffer, size_t length);
  LacewingFunction    size_t  lw_stream_write_partial         (lw_stream *, const char * buffer, size_t length);
  LacewingFunction      void  lw_stream_write_text            (lw_stream *, const char * buffer);
  LacewingFunction      void  lw_stream_writef                (lw_stream *, const char * format, ...);
  LacewingFunction      void  lw_stream_write_stream          (lw_stream *, lw_stream * src, size_t size, lw_bool delete_when_finished);
  LacewingFunction      void  lw_stream_write_file            (lw_stream * stream, const char * filename);
  LacewingFunction      void  lw_stream_add_filter_upstream   (lw_stream * stream, lw_stream * filter, lw_bool close_together);
  LacewingFunction      void  lw_stream_add_filter_downstream (lw_stream * stream, lw_stream * filter, lw_bool close_together);
  LacewingFunction   lw_bool  lw_stream_is_transparent        (lw_stream * stream);
  LacewingFunction      void* lw_stream_cast                  (lw_stream * stream, void * type);
  LacewingFunction      void  lw_stream_close                 (lw_stream * stream, lw_bool immediate);
  LacewingFunction      void  lw_stream_delete                (lw_stream * stream);

  typedef void (LacewingHandler * lw_stream_handler_data) (lw_stream *, void * tag, char * buffer, size_t length);

  LacewingFunction void lw_stream_add_handler_data (lw_stream *, lw_stream_handler_data, void * tag);
  LacewingFunction void lw_stream_remove_handler_data (lw_stream *, lw_stream_handler_data, void * tag);

  LacewingFunction void lw_stream_add_handler_close (lw_stream *, lw_stream_handler_data, void * tag);
  LacewingFunction void lw_stream_remove_handler_close (lw_stream *, lw_stream_handler_data, void * tag);

/* FDStream */

  LacewingFunction  lw_stream* lw_fdstream_new         (lw_pump *);
  LacewingFunction       void  lw_fdstream_set_fd      (lw_stream *, lw_fd fd, lw_pump_watch * watch);
  LacewingFunction       void  lw_fdstream_cork        (lw_stream *);
  LacewingFunction       void  lw_fdstream_uncork      (lw_stream *);
  LacewingFunction       void  lw_fdstream_nagle       (lw_stream *, lw_bool nagle);

/* File */

  LacewingFunction lw_stream * lw_file_new (lw_pump *);

  LacewingFunction lw_stream * lw_file_open_new
      (lw_pump *, const char * filename, const char * mode);

  LacewingFunction lw_bool lw_file_open
      (lw_stream * file, const char * filename, const char * mode);

/* Pipe */
  
  LacewingFunction      lw_stream* lw_pipe_new            ();
  LacewingFunction      lw_stream* lw_pipe_new_pump       (lw_pump *);

/* Timer */

  LacewingFlat (lw_timer);
  
  LacewingFunction       lw_timer* lw_timer_new                  ();
  LacewingFunction           void  lw_timer_delete               (lw_timer *);
  LacewingFunction           void  lw_timer_start                (lw_timer *, long milliseconds);
  LacewingFunction        lw_bool  lw_timer_started              (lw_timer *);
  LacewingFunction           void  lw_timer_stop                 (lw_timer *);
  LacewingFunction           void  lw_timer_force_tick           (lw_timer *);

  typedef void (LacewingHandler * lw_timer_handler_tick) (lw_timer *);
  LacewingFunction void lw_timer_ontick (lw_timer *, lw_timer_handler_tick);

/* Sync */

  LacewingFlat (lw_sync);
  LacewingFlat (lw_sync_lock);
  
  LacewingFunction        lw_sync* lw_sync_new                  ();
  LacewingFunction           void  lw_sync_delete               (lw_sync *);
  LacewingFunction   lw_sync_lock* lw_sync_lock_new             (lw_sync *);
  LacewingFunction           void  lw_sync_lock_delete          (lw_sync_lock *);
  LacewingFunction           void  lw_sync_lock_release         (lw_sync_lock *);

/* Event */

  LacewingFlat (lw_event);

  LacewingFunction       lw_event* lw_event_new                 ();
  LacewingFunction           void  lw_event_delete              (lw_event *);
  LacewingFunction           void  lw_event_signal              (lw_event *);
  LacewingFunction           void  lw_event_unsignal            (lw_event *);
  LacewingFunction        lw_bool  lw_event_signalled           (lw_event *);
  LacewingFunction           void  lw_event_wait                (lw_event *, long milliseconds);

/* Error */

  LacewingFlat (lw_error);

  LacewingFunction       lw_error* lw_error_new                 ();
  LacewingFunction           void  lw_error_delete              (lw_error *);
  LacewingFunction           void  lw_error_add                 (lw_error *, long);
  LacewingFunction           void  lw_error_addf                (lw_error *, const char * format, ...);
  LacewingFunction     const char* lw_error_tostring            (lw_error *);
  LacewingFunction       lw_error* lw_error_clone               (lw_error *);

/* Client */

  LacewingFunction      lw_stream* lw_client_new                (lw_pump *);
  LacewingFunction           void  lw_client_connect            (lw_stream *, const char * host, long port);
  LacewingFunction           void  lw_client_connect_addr       (lw_stream *, lw_addr *);
  LacewingFunction           void  lw_client_disconnect         (lw_stream *);
  LacewingFunction        lw_bool  lw_client_connected          (lw_stream *);
  LacewingFunction        lw_bool  lw_client_connecting         (lw_stream *);
  LacewingFunction        lw_addr* lw_client_server_addr        (lw_stream *);
  
  typedef void (LacewingHandler * lw_client_handler_connect) (lw_stream *);
  LacewingFunction void lw_client_onconnect (lw_stream *, lw_client_handler_connect);

  typedef void (LacewingHandler * lw_client_handler_disconnect) (lw_stream *);
  LacewingFunction void lw_client_ondisconnect (lw_stream *, lw_client_handler_disconnect);

  typedef void (LacewingHandler * lw_client_handler_receive) (lw_stream *, const char * buffer, long size);
  LacewingFunction void lw_client_onreceive (lw_stream *, lw_client_handler_receive);

  typedef void (LacewingHandler * lw_client_handler_error) (lw_stream *, lw_error *);
  LacewingFunction void lw_client_onerror (lw_stream *, lw_client_handler_error);

/* Server */

  LacewingFlat (lw_server);

  LacewingFunction        lw_server* lw_server_new                      (lw_pump *);
  LacewingFunction             void  lw_server_delete                   (lw_server *);
  LacewingFunction             void  lw_server_host                     (lw_server *, long port);
  LacewingFunction             void  lw_server_host_filter              (lw_server *, lw_filter * filter);
  LacewingFunction             void  lw_server_unhost                   (lw_server *);
  LacewingFunction          lw_bool  lw_server_hosting                  (lw_server *);
  LacewingFunction             long  lw_server_port                     (lw_server *);
  LacewingFunction          lw_bool  lw_server_load_cert_file           (lw_server *, const char * filename, const char * passphrase);
  LacewingFunction          lw_bool  lw_server_load_sys_cert            (lw_server *, const char * store_name, const char * common_name, const char * location);
  LacewingFunction          lw_bool  lw_server_cert_loaded              (lw_server *);
  LacewingFunction          lw_addr* lw_server_client_address           (lw_stream * client);
  LacewingFunction        lw_stream* lw_server_client_next              (lw_stream * client);

  typedef void (LacewingHandler * lw_server_handler_connect) (lw_server *, lw_stream * client);
  LacewingFunction void lw_server_onconnect (lw_server *, lw_server_handler_connect);

  typedef void (LacewingHandler * lw_server_handler_disconnect) (lw_server *, lw_stream * client);
  LacewingFunction void lw_server_ondisconnect (lw_server *, lw_server_handler_disconnect);

  typedef void (LacewingHandler * lw_server_handler_receive) (lw_server *, lw_stream * client, const char * buffer, size_t size);
  LacewingFunction void lw_server_onreceive (lw_server *, lw_server_handler_receive);
  
  typedef void (LacewingHandler * lw_server_handler_error) (lw_server *, lw_error *);
  LacewingFunction void lw_server_onerror (lw_server *, lw_server_handler_error);

/* UDP */

  LacewingFlat (lw_udp);

  LacewingFunction         lw_udp* lw_udp_new                   (lw_pump *);
  LacewingFunction           void  lw_udp_delete                (lw_udp *);
  LacewingFunction           void  lw_udp_host                  (lw_udp *, long port);
  LacewingFunction           void  lw_udp_host_filter           (lw_udp *, lw_filter *);
  LacewingFunction           void  lw_udp_host_addr             (lw_udp *, lw_addr *);
  LacewingFunction        lw_bool  lw_udp_hosting               (lw_udp *);
  LacewingFunction           void  lw_udp_unhost                (lw_udp *);
  LacewingFunction           long  lw_udp_port                  (lw_udp *);
  LacewingFunction           void  lw_udp_write                 (lw_udp *, lw_addr *, const char * buffer, size_t size);

  typedef void (LacewingHandler * lw_udp_handler_receive)(lw_udp *, lw_addr *, const char * buffer, size_t size);
  LacewingFunction void lw_udp_onreceive (lw_udp *, lw_udp_handler_receive);

  typedef void (LacewingHandler * lw_udp_handler_error) (lw_udp *, lw_error *);
  LacewingFunction void lw_udp_onerror (lw_udp *, lw_udp_handler_error);

/* FlashPolicy */

  LacewingFlat (lw_flashpolicy);

  LacewingFunction  lw_flashpolicy* lw_flashpolicy_new           (lw_pump *);
  LacewingFunction            void  lw_flashpolicy_delete        (lw_flashpolicy *);
  LacewingFunction            void  lw_flashpolicy_host          (lw_flashpolicy *, const char * filename);
  LacewingFunction            void  lw_flashpolicy_host_filter   (lw_flashpolicy *, const char * filename, lw_filter *);
  LacewingFunction            void  lw_flashpolicy_unhost        (lw_flashpolicy *);
  LacewingFunction         lw_bool  lw_flashpolicy_hosting       (lw_flashpolicy *);

  typedef void (LacewingHandler * lw_flashpolicy_handler_error) (lw_flashpolicy *, lw_error *);
  LacewingFunction void lw_flashpolicy_onerror (lw_flashpolicy *, lw_flashpolicy_handler_error);

/* Webserver */

  LacewingFlat (lw_ws);
  LacewingFlat (lw_ws_req_hdr);
  LacewingFlat (lw_ws_req_param);
  LacewingFlat (lw_ws_req_cookie);
  LacewingFlat (lw_ws_req_ssnitem);
  LacewingFlat (lw_ws_upload);
  LacewingFlat (lw_ws_upload_hdr);

  LacewingFunction              lw_ws* lw_ws_new                    (lw_pump *);
  LacewingFunction               void  lw_ws_delete                 (lw_ws *);
  LacewingFunction               void  lw_ws_host                   (lw_ws *, long port);
  LacewingFunction               void  lw_ws_host_secure            (lw_ws *, long port);
  LacewingFunction               void  lw_ws_host_filter            (lw_ws *, lw_filter *);
  LacewingFunction               void  lw_ws_host_secure_filter     (lw_ws *, lw_filter *);
  LacewingFunction               void  lw_ws_unhost                 (lw_ws *);
  LacewingFunction               void  lw_ws_unhost_secure          (lw_ws *);
  LacewingFunction            lw_bool  lw_ws_hosting                (lw_ws *);
  LacewingFunction            lw_bool  lw_ws_hosting_secure         (lw_ws *);
  LacewingFunction               long  lw_ws_port                   (lw_ws *);
  LacewingFunction               long  lw_ws_port_secure            (lw_ws *);
  LacewingFunction            lw_bool  lw_ws_load_cert_file         (lw_ws *, const char * filename, const char * passphrase);
  LacewingFunction            lw_bool  lw_ws_load_sys_cert          (lw_ws *, const char * store_name, const char * common_name, const char * location);
  LacewingFunction            lw_bool  lw_ws_cert_loaded            (lw_ws *);
  LacewingFunction               void  lw_ws_close_session          (lw_ws *, const char * id);
  LacewingFunction               void  lw_ws_enable_manual_finish   (lw_ws *);
  LacewingFunction               long  lw_ws_idle_timeout           (lw_ws *);
  LacewingFunction               void  lw_ws_set_idle_timeout       (lw_ws *, long seconds);  
  LacewingFunction            lw_addr* lw_ws_req_addr               (lw_stream * req);
  LacewingFunction            lw_bool  lw_ws_req_secure             (lw_stream * req);
  LacewingFunction         const char* lw_ws_req_url                (lw_stream * req);
  LacewingFunction         const char* lw_ws_req_hostname           (lw_stream * req);
  LacewingFunction               void  lw_ws_req_disconnect         (lw_stream * req); 
  LacewingFunction               void  lw_ws_req_set_redirect       (lw_stream * req, const char * url);
  LacewingFunction               void  lw_ws_req_set_status         (lw_stream * req, long code, const char * message);
  LacewingFunction               void  lw_ws_req_set_mime_type      (lw_stream * req, const char * mime_type);
  LacewingFunction               void  lw_ws_req_set_mime_type_ex   (lw_stream * req, const char * mime_type, const char * charset);
  LacewingFunction               void  lw_ws_req_guess_mime_type    (lw_stream * req, const char * filename);
  LacewingFunction               void  lw_ws_req_finish             (lw_stream * req);
  LacewingFunction             lw_i64  lw_ws_req_last_modified      (lw_stream * req);
  LacewingFunction               void  lw_ws_req_set_last_modified  (lw_stream * req, lw_i64);
  LacewingFunction               void  lw_ws_req_set_unmodified     (lw_stream * req);
  LacewingFunction               void  lw_ws_req_set_header         (lw_stream * req, const char * name, const char * value);
  LacewingFunction         const char* lw_ws_req_header             (lw_stream * req, const char * name);
  LacewingFunction      lw_ws_req_hdr* lw_ws_req_first_header       (lw_stream * req);
  LacewingFunction         const char* lw_ws_req_hdr_name           (lw_ws_req_hdr *);
  LacewingFunction         const char* lw_ws_req_hdr_value          (lw_ws_req_hdr *);
  LacewingFunction      lw_ws_req_hdr* lw_ws_req_hdr_next           (lw_ws_req_hdr *);
  LacewingFunction    lw_ws_req_param* lw_ws_req_first_GET          (lw_stream * req);
  LacewingFunction    lw_ws_req_param* lw_ws_req_first_POST         (lw_stream * req);
  LacewingFunction         const char* lw_ws_req_param_name         (lw_ws_req_param *);
  LacewingFunction         const char* lw_ws_req_param_value        (lw_ws_req_param *);
  LacewingFunction    lw_ws_req_param* lw_ws_req_param_next         (lw_ws_req_param *);
  LacewingFunction   lw_ws_req_cookie* lw_ws_req_first_cookie       (lw_stream * req);
  LacewingFunction         const char* lw_ws_req_cookie_name        (lw_ws_req_cookie *);
  LacewingFunction         const char* lw_ws_req_cookie_value       (lw_ws_req_cookie *);
  LacewingFunction   lw_ws_req_cookie* lw_ws_req_cookie_next        (lw_ws_req_cookie *);
  LacewingFunction               void  lw_ws_req_set_cookie         (lw_stream * req, const char * name, const char * value);
  LacewingFunction               void  lw_ws_req_set_cookie_ex      (lw_stream * req, const char * name, const char * value, const char * attributes);
  LacewingFunction         const char* lw_ws_req_get_cookie         (lw_stream * req, const char * name);
  LacewingFunction         const char* lw_ws_req_session_id         (lw_stream * req);
  LacewingFunction               void  lw_ws_req_session_write      (lw_stream * req, const char * name, const char * value);
  LacewingFunction         const char* lw_ws_req_session_read       (lw_stream * req, const char * name);
  LacewingFunction               void  lw_ws_req_session_close      (lw_stream * req);
  LacewingFunction  lw_ws_req_ssnitem* lw_ws_req_first_session_item (lw_stream * req);
  LacewingFunction         const char* lw_ws_req_ssnitem_name       (lw_ws_req_ssnitem *);
  LacewingFunction         const char* lw_ws_req_ssnitem_value      (lw_ws_req_ssnitem *);
  LacewingFunction  lw_ws_req_ssnitem* lw_ws_req_ssnitem_next       (lw_ws_req_ssnitem *);
  LacewingFunction         const char* lw_ws_req_GET                (lw_stream * req, const char * name);
  LacewingFunction         const char* lw_ws_req_POST               (lw_stream * req, const char * name);
  LacewingFunction         const char* lw_ws_req_body               (lw_stream * req);
  LacewingFunction               void  lw_ws_req_disable_cache      (lw_stream * req);
  LacewingFunction               long  lw_ws_req_idle_timeout       (lw_stream * req);
  LacewingFunction               void  lw_ws_req_set_idle_timeout   (lw_stream * req, long seconds);  
/*LacewingFunction               void  lw_ws_req_enable_dl_resuming (lw_stream * req);
  LacewingFunction             lw_i64  lw_ws_req_reqrange_begin     (lw_stream * req);
  LacewingFunction             lw_i64  lw_ws_req_reqrange_end       (lw_stream * req);
  LacewingFunction               void  lw_ws_req_set_outgoing_range (lw_stream * req, lw_i64 begin, lw_i64 end);*/
  LacewingFunction         const char* lw_ws_upload_form_el_name    (lw_ws_upload *);
  LacewingFunction         const char* lw_ws_upload_filename        (lw_ws_upload *);
  LacewingFunction         const char* lw_ws_upload_header          (lw_ws_upload *, const char * name);
  LacewingFunction               void  lw_ws_upload_set_autosave    (lw_ws_upload *);
  LacewingFunction         const char* lw_ws_upload_autosave_fname  (lw_ws_upload *);
  LacewingFunction   lw_ws_upload_hdr* lw_ws_upload_first_header    (lw_ws_upload *);
  LacewingFunction         const char* lw_ws_upload_hdr_name        (lw_ws_upload_hdr *);
  LacewingFunction         const char* lw_ws_upload_hdr_value       (lw_ws_upload_hdr *);
  LacewingFunction   lw_ws_upload_hdr* lw_ws_upload_hdr_next        (lw_ws_upload_hdr *);

  typedef void (LacewingHandler * lw_ws_handler_get) (lw_ws *, lw_stream * req);
  LacewingFunction void lw_ws_onget (lw_ws *, lw_ws_handler_get);

  typedef void (LacewingHandler * lw_ws_handler_post) (lw_ws *, lw_stream * req);
  LacewingFunction void lw_ws_onpost (lw_ws *, lw_ws_handler_post);

  typedef void (LacewingHandler * lw_ws_handler_head) (lw_ws *, lw_stream * req);
  LacewingFunction void lw_ws_onhead (lw_ws *, lw_ws_handler_head);

  typedef void (LacewingHandler * lw_ws_handler_error) (lw_ws *, lw_error *);
  LacewingFunction void lw_ws_onerror (lw_ws *, lw_ws_handler_error);

  typedef void (LacewingHandler * lw_ws_handler_disconnect) (lw_ws *, lw_stream * req);
  LacewingFunction void lw_ws_ondisconnect (lw_ws *, lw_ws_handler_disconnect);

  typedef void (LacewingHandler * lw_ws_handler_upload_start) (lw_ws *, lw_stream * req, lw_ws_upload *);
  LacewingFunction void lw_ws_onuploadstart (lw_ws *, lw_ws_handler_upload_start);

  typedef void (LacewingHandler * lw_ws_handler_upload_chunk) (lw_ws *, lw_stream * req, lw_ws_upload *, const char * buffer, size_t size);
  LacewingFunction void lw_ws_onuploadchunk (lw_ws *, lw_ws_handler_upload_chunk);

  typedef void (LacewingHandler * lw_ws_handler_upload_done) (lw_ws *, lw_stream * req, lw_ws_upload *);
  LacewingFunction void lw_ws_onuploaddone (lw_ws *, lw_ws_handler_upload_done);

  typedef void (LacewingHandler * lw_ws_handler_upload_post) (lw_ws *, lw_stream * req, lw_ws_upload * uploads [], long upload_count);
  LacewingFunction void lw_ws_onuploadpost (lw_ws *, lw_ws_handler_upload_post);


#ifdef __cplusplus
}

#define LacewingClass \
    struct Internal;  Internal * internal; 

#define LacewingClassTag \
    struct Internal;  Internal * internal;  void * Tag;

namespace Lacewing
{

struct Error
{
    LacewingClassTag;

    LacewingFunction  Error ();
    LacewingFunction ~Error ();
    
    LacewingFunction void Add (const char * Format, ...);
    LacewingFunction void Add (int);
    LacewingFunction void Add (const char * Format, va_list);

    LacewingFunction int Size ();

    LacewingFunction const char * ToString ();
    LacewingFunction operator const char * ();

    LacewingFunction Lacewing::Error * Clone ();
};

struct Event
{
    LacewingClassTag;

    LacewingFunction Event ();
    LacewingFunction ~Event ();

    LacewingFunction void Signal ();
    LacewingFunction void Unsignal ();
    
    LacewingFunction bool Signalled ();

    LacewingFunction bool Wait (int Timeout = -1);
};

struct Pump
{
private:

    LacewingClassTag;

public:

    LacewingFunction Pump ();
    LacewingFunction virtual ~Pump ();

    LacewingFunction void AddUser ();
    LacewingFunction void RemoveUser ();

    LacewingFunction bool InUse ();

    struct Watch;

    #ifdef _WIN32

        typedef void (* Callback)
            (void * Tag, OVERLAPPED *, unsigned int BytesTransferred, int Error);

        virtual Watch * Add (HANDLE, void * tag, Callback) = 0;

        virtual void UpdateCallbacks (Watch *, void * tag, Callback) = 0;

    #else

        typedef void (* Callback) (void * Tag);

        virtual Watch * Add
            (int FD, void * Tag, Callback onReadReady,
                Callback onWriteReady = 0, bool edge_triggered = true) = 0;

        virtual void UpdateCallbacks
            (Watch *, void * tag, Callback onReadReady,
                Callback onWriteReady = 0, bool edge_triggered = true) = 0;

    #endif
 
    virtual void Remove (Watch *) = 0;

    virtual void Post (void * Function, void * Parameter = 0) = 0;   

    LacewingFunction virtual bool IsEventPump ();
};

struct EventPump : public Pump
{
    LacewingClassTag;

    LacewingFunction  EventPump (int MaxHint = 1024);
    LacewingFunction ~EventPump ();

    LacewingFunction Lacewing::Error * Tick ();
    LacewingFunction Lacewing::Error * StartEventLoop ();

    LacewingFunction Lacewing::Error * StartSleepyTicking
        (void (LacewingHandler * onTickNeeded) (Lacewing::EventPump &EventPump));

    LacewingFunction void PostEventLoopExit ();

    #ifdef _WIN32

        LacewingFunction Watch * Add (HANDLE, void * tag, Callback);

        LacewingFunction void UpdateCallbacks (Watch *, void * tag, Callback);

    #else

        LacewingFunction Watch * Add
            (int FD, void * Tag, Callback onReadReady,
                Callback onWriteReady = 0, bool edge_triggered = true);

        LacewingFunction void UpdateCallbacks
            (Watch *, void * tag, Callback onReadReady,
                Callback onWriteReady = 0, bool edge_triggered = true);

    #endif

    LacewingFunction void Remove (Watch *);

    LacewingFunction void Post (void * Function, void * Parameter = 0);

    LacewingFunction bool IsEventPump ();
};

struct Thread
{
    LacewingClassTag;

    LacewingFunction   Thread (const char * Name, void * Function);
    LacewingFunction ~ Thread ();

    LacewingFunction void Start (void * Parameter = 0);
    LacewingFunction bool Started ();

    LacewingFunction int Join ();
};

struct Timer
{
    LacewingClassTag;

    LacewingFunction  Timer (Pump &);
    LacewingFunction ~Timer ();

    LacewingFunction void Start    (int Milliseconds);
    LacewingFunction void Stop     ();
    LacewingFunction bool Started  ();
    
    LacewingFunction void ForceTick ();
    
    typedef void (LacewingHandler * HandlerTick) (Lacewing::Timer &Timer);
    LacewingFunction void onTick (HandlerTick);
};

struct Sync
{
    LacewingClassTag;

    LacewingFunction  Sync ();
    LacewingFunction ~Sync ();

    struct Lock
    {
        void * internal;

        LacewingFunction  Lock (Lacewing::Sync &);
        LacewingFunction ~Lock ();
        
        LacewingFunction void Release ();
    };
};

struct Stream
{
    LacewingClass;

    LacewingFunction Stream ();
    LacewingFunction Stream (Pump &);

    LacewingFunction virtual ~ Stream ();

    typedef void (LacewingHandler * HandlerData)
        (Stream &, void * tag, const char * buffer, size_t size);

    LacewingFunction void AddHandlerData (HandlerData, void * tag = 0);
    LacewingFunction void RemoveHandlerData (HandlerData, void * tag = 0);

    typedef void (LacewingHandler * HandlerClose) (Stream &, void * Tag);

    LacewingFunction void AddHandlerClose (HandlerClose, void * tag);
    LacewingFunction void RemoveHandlerClose (HandlerClose, void * tag);

    LacewingFunction virtual size_t BytesLeft (); /* if -1, Read() does nothing */
    LacewingFunction virtual void Read (size_t bytes = -1); /* -1 = until EOF */

    LacewingFunction void BeginQueue ();
    LacewingFunction size_t Queued ();

    /* When EndQueue is called, one or more head buffers may optionally be
     * written _before_ the queued data.  This is used for e.g. including HTTP
     * headers before the (already buffered) response body.
     */

    LacewingFunction void EndQueue ();    

    LacewingFunction void EndQueue
        (int head_buffers, const char ** buffers, size_t * lengths);

    LacewingFunction void ClearQueue ();

    LacewingFunction void Write
        (const char * buffer, size_t size = -1);

    LacewingFunction size_t WritePartial
        (const char * buffer, size_t size = -1);

    inline Stream &operator << (char * s)                   
    {   Write (s);                                      
        return *this;                                   
    }                                                   

    inline Stream &operator << (const char * s)             
    {   Write (s);                                      
        return *this;                                   
    }                                                   

    inline Stream &operator << (lw_i64 v)                   
    {   char buffer [128];
        sprintf (buffer, "%" lw_PRId64, v);
        Write (buffer);
        return *this;                                   
    }                                                   

    LacewingFunction void Write
        (Stream &, size_t size = -1, bool delete_when_finished = false);

    LacewingFunction void WriteFile (const char * filename);

    LacewingFunction void AddFilterUpstream
      (Stream &, bool delete_with_stream = false, bool close_together = false);

    LacewingFunction void AddFilterDownstream
      (Stream &, bool delete_with_stream = false, bool close_together = false);

    LacewingFunction virtual bool IsTransparent ();

    LacewingFunction virtual bool Close (bool immediate = false);

    /* Since we don't compile with RTTI (and this is the only place it would be needed) */

    LacewingFunction virtual void * Cast (void * type);

protected:
 
    LacewingFunction void Data (const char * buffer, size_t size);

    const static int Retry_Now = 1,  Retry_Never = 2,  Retry_MoreData = 3;
    LacewingFunction virtual void Retry (int when = Retry_Now);

    LacewingFunction virtual size_t Put (const char * buffer, size_t size) = 0;

    LacewingFunction virtual size_t Put (Stream &, size_t size);
};
              
struct Pipe : public Stream
{
    LacewingFunction Pipe ();
    LacewingFunction Pipe (Lacewing::Pump &);

    LacewingFunction ~ Pipe ();

    LacewingFunction size_t Put (const char * buffer, size_t size);

    LacewingFunction bool IsTransparent ();
};

/* TODO : Seek method? */

struct FDStream : public Stream
{
private:

    LacewingClass;

public:

    using Stream::Write;

    LacewingFunction FDStream (Lacewing::Pump &);
    LacewingFunction virtual ~ FDStream ();

    LacewingFunction void SetFD
        (lw_fd, Pump::Watch * watch = 0, bool auto_close = false);

    LacewingFunction bool Valid ();

    LacewingFunction virtual size_t BytesLeft ();
    LacewingFunction virtual void Read (size_t Bytes = -1);

    LacewingFunction void Cork ();
    LacewingFunction void Uncork ();    

    LacewingFunction void Nagle (bool);

    LacewingFunction bool Close (bool immediate = false);

    LacewingFunction virtual void * Cast (void *);

protected:

    LacewingFunction size_t Put (const char * buffer, size_t size);
    LacewingFunction size_t Put (Stream &, size_t size);
};

struct File : public FDStream
{
    LacewingClassTag;

    LacewingFunction File
        (Lacewing::Pump &);

    LacewingFunction File
        (Lacewing::Pump &, const char * filename, const char * mode = "rb");
    
    LacewingFunction virtual ~ File ();

    LacewingFunction bool Open (const char * filename, const char * mode = "rb");
    LacewingFunction bool OpenTemp ();

    LacewingFunction const char * Name ();
};

struct Address
{  
    LacewingClassTag;

    const static int HINT_TCP   = 1;
    const static int HINT_UDP   = 2;
    const static int HINT_IPv4  = 4;

    LacewingFunction Address (Address &);
    LacewingFunction Address (const char * Hostname, int Port = 0, int Hints = 0);
    LacewingFunction Address (const char * Hostname, const char * Service, int Hints = 0);

    LacewingFunction ~ Address ();

    LacewingFunction  int Port ();
    LacewingFunction void Port (int);
    
    LacewingFunction bool IPv6 ();

    LacewingFunction bool Ready ();
    LacewingFunction Lacewing::Error * Resolve ();

    LacewingFunction const char * ToString  ();
    LacewingFunction operator const char *  ();

    LacewingFunction bool operator == (Address &);
    LacewingFunction bool operator != (Address &);
};

struct Filter
{
    LacewingClassTag;

    LacewingFunction Filter ();
    LacewingFunction Filter (Filter &);

    LacewingFunction ~Filter ();

    LacewingFunction void Local (Lacewing::Address *);    
    LacewingFunction void Remote (Lacewing::Address *);

    LacewingFunction Lacewing::Address * Local ();    
    LacewingFunction Lacewing::Address * Remote (); 

    LacewingFunction void LocalPort (int Port);   
    LacewingFunction  int LocalPort ();    

    LacewingFunction void RemotePort (int Port);   
    LacewingFunction  int RemotePort ();    

    LacewingFunction void Reuse (bool Enabled);
    LacewingFunction bool Reuse ();

    LacewingFunction void IPv6 (bool Enabled);
    LacewingFunction bool IPv6 ();
};

struct Client : public FDStream
{
    LacewingClassTag;

    LacewingFunction  Client (Pump &);
    LacewingFunction ~Client ();

    LacewingFunction void Connect (const char * Host, int Port);
    LacewingFunction void Connect (Address &);

    LacewingFunction bool Connected ();
    LacewingFunction bool Connecting ();

    LacewingFunction void Disconnect ();
    
    LacewingFunction Lacewing::Address &ServerAddress ();

    typedef void (LacewingHandler * HandlerConnect)
        (Lacewing::Client &client);

    typedef void (LacewingHandler * HandlerDisconnect)
        (Lacewing::Client &client);

    typedef void (LacewingHandler * HandlerReceive)
        (Lacewing::Client &client, const char * buffer, size_t size);

    typedef void (LacewingHandler * HandlerError)
        (Lacewing::Client &client, Lacewing::Error &error);

    LacewingFunction void onConnect    (HandlerConnect);
    LacewingFunction void onDisconnect (HandlerDisconnect);
    LacewingFunction void onReceive    (HandlerReceive);
    LacewingFunction void onError      (HandlerError);
};

struct Server
{
    LacewingClassTag;

    LacewingFunction  Server (Pump &);
    LacewingFunction ~Server ();

    LacewingFunction void Host    (int Port);
    LacewingFunction void Host    (Lacewing::Filter &Filter);

    LacewingFunction void Unhost  ();
    LacewingFunction bool Hosting ();
    LacewingFunction int  Port    ();

    LacewingFunction bool LoadCertificateFile
        (const char * filename, const char * passphrase = "");

    LacewingFunction bool LoadSystemCertificate
        (const char * storeName, const char * common_name,
         const char * location = "CurrentUser");

    LacewingFunction bool CertificateLoaded ();

    LacewingFunction bool CanNPN ();
    LacewingFunction void AddNPN (const char *);

    struct Client : public FDStream
    {
        Client (Lacewing::Pump &, lw_fd);

        LacewingClassTag;

        LacewingFunction Lacewing::Address &GetAddress ();

        LacewingFunction Client * Next ();

        LacewingFunction const char * NPN ();
    };

    LacewingFunction int ClientCount ();
    LacewingFunction Client * FirstClient ();

    typedef void (LacewingHandler * HandlerConnect)
        (Lacewing::Server &server, Lacewing::Server::Client &client);

    typedef void (LacewingHandler * HandlerDisconnect)
        (Lacewing::Server &server, Lacewing::Server::Client &client);

    typedef void (LacewingHandler * HandlerReceive)
        (Lacewing::Server &server, Lacewing::Server::Client &client,
             const char * data, size_t size);

    typedef void (LacewingHandler * HandlerError)
        (Lacewing::Server &server, Lacewing::Error &error);
    
    LacewingFunction void onConnect     (HandlerConnect);
    LacewingFunction void onDisconnect  (HandlerDisconnect);
    LacewingFunction void onReceive     (HandlerReceive);
    LacewingFunction void onError       (HandlerError);
};

struct UDP
{
    LacewingClassTag;

    LacewingFunction  UDP (Pump &);
    LacewingFunction ~UDP ();

    LacewingFunction void Host (int Port);
    LacewingFunction void Host (Lacewing::Filter &Filter);
    LacewingFunction void Host (Address &); /* Use Port () afterwards to get the port number */

    LacewingFunction bool Hosting ();
    LacewingFunction void Unhost ();

    LacewingFunction int Port ();

    LacewingFunction void Write (Lacewing::Address &Address, const char * Data, size_t Size = -1);

    typedef void (LacewingHandler * HandlerReceive)
        (Lacewing::UDP &UDP, Lacewing::Address &from, char * data, size_t size);

    typedef void (LacewingHandler * HandlerError)
        (Lacewing::UDP &UDP, Lacewing::Error &error);
    
    LacewingFunction void onReceive (HandlerReceive);
    LacewingFunction void onError  (HandlerError);
};

struct Webserver
{
    LacewingClassTag;

    LacewingFunction  Webserver (Pump &);
    LacewingFunction ~Webserver ();

    LacewingFunction void Host         (int Port = 80);
    LacewingFunction void HostSecure   (int Port = 443);
    
    LacewingFunction void Host         (Lacewing::Filter &Filter);
    LacewingFunction void HostSecure   (Lacewing::Filter &Filter);
    
    LacewingFunction void Unhost        ();
    LacewingFunction void UnhostSecure  ();

    LacewingFunction bool Hosting       ();
    LacewingFunction bool HostingSecure ();

    LacewingFunction int  Port          ();
    LacewingFunction int  SecurePort    ();

    LacewingFunction bool LoadCertificateFile
        (const char * filename, const char * passphrase = "");

    LacewingFunction bool LoadSystemCertificate
        (const char * storeName, const char * common_name,
         const char * location = "CurrentUser");

    LacewingFunction bool CertificateLoaded ();

    LacewingFunction void EnableManualRequestFinish ();

    LacewingFunction int  IdleTimeout ();
    LacewingFunction void IdleTimeout (int Seconds);

    struct Request : public Stream
    {
        LacewingClassTag;

        LacewingFunction Request (Pump &);
        LacewingFunction ~ Request ();

        LacewingFunction Address &GetAddress ();

        LacewingFunction bool Secure ();

        LacewingFunction const char * URL ();
        LacewingFunction const char * Hostname ();
        
        LacewingFunction void Disconnect ();

        LacewingFunction void SetRedirect (const char * URL);
        LacewingFunction void Status (int Code, const char * Message);

        LacewingFunction void SetMimeType (const char * MimeType, const char * Charset = "UTF-8");
        LacewingFunction void GuessMimeType (const char * Filename);

        LacewingFunction void Finish ();

        LacewingFunction int  IdleTimeout ();
        LacewingFunction void IdleTimeout (int Seconds);

        LacewingFunction lw_i64 LastModified  ();
        LacewingFunction void   LastModified  (lw_i64 Time);
        LacewingFunction void   SetUnmodified ();
    
        LacewingFunction void DisableCache ();

        LacewingFunction void   EnableDownloadResuming ();
        LacewingFunction lw_i64 RequestedRangeBegin ();
        LacewingFunction lw_i64 RequestedRangeEnd ();
        LacewingFunction void   SetOutgoingRange (lw_i64 Begin, lw_i64 End);
        

        /** Headers **/

        struct Header
        {
            LacewingFunction const char * Name ();
            LacewingFunction const char * Value ();

            LacewingFunction Header * Next ();
        };

        LacewingFunction struct Header * FirstHeader ();

        LacewingFunction const char * Header (const char * Name);
        LacewingFunction void Header (const char * Name, const char * Value);

        /* Does not overwrite an existing header with the same name */

        LacewingFunction void AddHeader
            (const char * Name, const char * Value);

    
        /** Cookies **/

        struct Cookie
        {
            LacewingFunction const char * Name ();
            LacewingFunction const char * Value ();

            LacewingFunction Cookie * Next ();
        };

        LacewingFunction struct Cookie * FirstCookie ();

        LacewingFunction const char * Cookie (const char * Name);
        LacewingFunction void         Cookie (const char * Name, const char * Value);
        LacewingFunction void         Cookie (const char * Name, const char * Value, const char * Attributes);

    
        /** Sessions **/

        struct SessionItem
        {
            LacewingFunction const char * Name ();
            LacewingFunction const char * Value ();

            LacewingFunction SessionItem * Next ();
        };

        LacewingFunction SessionItem * FirstSessionItem ();

        LacewingFunction const char * Session ();
        LacewingFunction const char * Session (const char * Key);
        LacewingFunction void         Session (const char * Key, const char * Value);

        LacewingFunction void  CloseSession ();

            
        /** GET/POST data **/

        struct Parameter
        {
            LacewingFunction const char * Name ();
            LacewingFunction const char * Value ();
            LacewingFunction const char * ContentType ();

            LacewingFunction Parameter * Next ();
        };

        LacewingFunction Parameter * GET ();
        LacewingFunction Parameter * POST ();

        LacewingFunction const char * GET  (const char * Name);
        LacewingFunction const char * POST (const char * Name);

        LacewingFunction const char * Body ();
    };

    LacewingFunction void CloseSession (const char * ID);

    struct Upload
    {
        LacewingClassTag;

        LacewingFunction const char * FormElementName ();
        LacewingFunction const char * Filename ();
        LacewingFunction void         SetAutoSave ();
        LacewingFunction const char * GetAutoSaveFilename ();

        LacewingFunction const char * Header (const char * Name);
        
        struct Header
        {
            LacewingFunction const char * Name ();
            LacewingFunction const char * Value ();

            LacewingFunction Header * Next ();
        };

        LacewingFunction struct Header * FirstHeader ();
    };

    typedef void (LacewingHandler * HandlerGet)                    (Lacewing::Webserver &Webserver, Lacewing::Webserver::Request &Request);
    typedef void (LacewingHandler * HandlerPost)                   (Lacewing::Webserver &Webserver, Lacewing::Webserver::Request &Request);
    typedef void (LacewingHandler * HandlerHead)                   (Lacewing::Webserver &Webserver, Lacewing::Webserver::Request &Request);  
    typedef void (LacewingHandler * HandlerDisconnect)             (Lacewing::Webserver &Webserver, Lacewing::Webserver::Request &Request);
    typedef void (LacewingHandler * HandlerError)                  (Lacewing::Webserver &Webserver, Lacewing::Error &);

    typedef void (LacewingHandler * HandlerUploadStart)            (Lacewing::Webserver &Webserver, Lacewing::Webserver::Request &Request,
                                                                    Lacewing::Webserver::Upload &Upload);

    typedef void (LacewingHandler * HandlerUploadChunk)            (Lacewing::Webserver &Webserver, Lacewing::Webserver::Request &Request,
                                                                    Lacewing::Webserver::Upload &Upload, const char * Data, size_t Size);
     
    typedef void (LacewingHandler * HandlerUploadDone)             (Lacewing::Webserver &Webserver, Lacewing::Webserver::Request &Request,
                                                                    Lacewing::Webserver::Upload &Upload);

    typedef void (LacewingHandler * HandlerUploadPost)             (Lacewing::Webserver &Webserver, Lacewing::Webserver::Request &Request,
                                                                    Lacewing::Webserver::Upload * Uploads [], int UploadCount);

    LacewingFunction void onGet              (HandlerGet);
    LacewingFunction void onUploadStart      (HandlerUploadStart);
    LacewingFunction void onUploadChunk      (HandlerUploadChunk);
    LacewingFunction void onUploadDone       (HandlerUploadDone);
    LacewingFunction void onUploadPost       (HandlerUploadPost);
    LacewingFunction void onPost             (HandlerPost);
    LacewingFunction void onHead             (HandlerHead);
    LacewingFunction void onDisconnect       (HandlerDisconnect);
    LacewingFunction void onError            (HandlerError);
};
    
struct FlashPolicy
{
    LacewingClassTag;

    LacewingFunction  FlashPolicy (Pump &);
    LacewingFunction ~FlashPolicy ();

    LacewingFunction void Host (const char * Filename);
    LacewingFunction void Host (const char * Filename, Lacewing::Filter &Filter);

    LacewingFunction void Unhost ();

    LacewingFunction bool Hosting ();

    typedef void (LacewingHandler * HandlerError)
        (Lacewing::FlashPolicy &FlashPolicy, Lacewing::Error &);

    LacewingFunction void onError (HandlerError);
};

}

#endif /* __cplusplus */
#endif /* LacewingIncluded */

