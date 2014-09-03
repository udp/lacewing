
cmake_minimum_required (VERSION 2.6)
project (lacewing)

include (CheckIncludeFiles)
include (CheckSymbolExists)
include (CheckFunctionExists)

option (ENABLE_CXX "Enable C++ API" ON)
option (ENABLE_SPDY "Enable SPDY support in webserver" OFF)
option (ENABLE_SSL "Enable SSL support" OFF)
option (ENABLE_THREADS "Enable thread support" ON)

set (CMAKE_C_FLAGS "-std=gnu99 -Wno-deprecated-declarations ${CMAKE_C_FLAGS}")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

include_directories(deps/libuv/include deps/libuv/src)

set (SOURCES ${SOURCES}
        src/global.c
        src/nvhash.c
        src/filter.c
        src/address.c
        src/streamgraph.c
        src/stream.c
        src/error.c
        src/webserver/webserver.c
        src/webserver/http/http-client.c
        src/webserver/http/http-parse.c
        src/webserver/mimetypes.c
        src/webserver/request.c
        src/webserver/sessions.c
        src/pipe.c
        src/webserver/multipart.c
        src/flashpolicy.c
        src/pump.c
        src/util.c
        src/list.c
        src/heapbuffer.c
        src/webserver/upload.c
        deps/multipart-parser/multipart_parser.c
        deps/http-parser/http_parser.c
        deps/libuv/src/fs-poll.c
        deps/libuv/src/inet.c
        deps/libuv/src/threadpool.c
        deps/libuv/src/uv-common.c
        deps/libuv/src/version.c
        )

if (ENABLE_CXX)
    set (SOURCES ${SOURCES}
            src/cxx/address.cc
            src/cxx/client.cc
            src/cxx/error.cc
            src/cxx/event.cc
            src/cxx/eventpump.cc
            src/cxx/fdstream.cc
            src/cxx/file.cc
            src/cxx/filter.cc
            src/cxx/flashpolicy.cc
            src/cxx/pipe.cc
            src/cxx/pump.cc
            src/cxx/server.cc
            src/cxx/stream.cc
            src/cxx/sync.cc
            src/cxx/timer.cc
            src/cxx/udp.cc
            src/cxx/webserver.cc)
endif ()

# Windows or UNIX?
#
if (WIN32)

    include_directories(deps/libuv/src/win)

    set (SOURCES ${SOURCES}
            src/windows/client.c
            src/windows/compat.c
            src/windows/event.c
            src/windows/eventpump.c
            src/windows/fdstream.c
            src/windows/file.c
            src/windows/global.c
            src/windows/server.c
            src/windows/sslclient.c
            src/windows/sync.c
            src/windows/thread.c
            src/windows/timer.c
            src/windows/udp.c
            deps/libuv/src/win/async.c
            deps/libuv/src/win/core.c
            deps/libuv/src/win/dl.c
            deps/libuv/src/win/error.c
            deps/libuv/src/win/fs.c
            deps/libuv/src/win/fs-event.c
            deps/libuv/src/win/getaddrinfo.c
            deps/libuv/src/win/getnameinfo.c
            deps/libuv/src/win/handle.c
            deps/libuv/src/win/loop-watcher.c
            deps/libuv/src/win/pipe.c
            deps/libuv/src/win/thread.c
            deps/libuv/src/win/poll.c
            deps/libuv/src/win/process.c
            deps/libuv/src/win/process-stdio.c
            deps/libuv/src/win/req.c
            deps/libuv/src/win/signal.c
            deps/libuv/src/win/stream.c
            deps/libuv/src/win/tcp.c
            deps/libuv/src/win/tty.c
            deps/libuv/src/win/timer.c
            deps/libuv/src/win/udp.c
            deps/libuv/src/win/util.c
            deps/libuv/src/win/winapi.c
            deps/libuv/src/win/winsock.c
            )

    set (LIBS ${LIBS} mswsock ws2_32 crypt32 secur32)
    set (LIBS ${LIBS} advapi32 iphlpapi psapi shell32)  # libuv

    # Use Windows native SSL rather than OpenSSL on Windows
    #
    if (ENABLE_SSL)

        set (SOURCES ${SOURCES}
                src/windows/ssl.c
                src/windows/clientssl.c
                src/windows/serverssl.c)

    endif (ENABLE_SSL)

elseif (UNIX)

    include_directories(deps/libuv/src/unix)

    set (SOURCES ${SOURCES}
            src/unix/client.c
            src/unix/event.c
            src/unix/eventpump.c
            src/unix/fdstream.c
            src/unix/file.c
            src/unix/global.c
            src/unix/server.c
            src/unix/sync.c
            src/unix/thread.c
            src/unix/timer.c
            src/unix/udp.c
            deps/libuv/src/unix/async.c
            deps/libuv/src/unix/core.c
            deps/libuv/src/unix/dl.c
            deps/libuv/src/unix/fs.c
            deps/libuv/src/unix/getaddrinfo.c
            deps/libuv/src/unix/getnameinfo.c
            deps/libuv/src/unix/loop.c
            deps/libuv/src/unix/loop-watcher.c
            deps/libuv/src/unix/pipe.c
            deps/libuv/src/unix/poll.c
            deps/libuv/src/unix/process.c
            deps/libuv/src/unix/signal.c
            deps/libuv/src/unix/stream.c
            deps/libuv/src/unix/tcp.c
            deps/libuv/src/unix/thread.c
            deps/libuv/src/unix/timer.c
            deps/libuv/src/unix/tty.c
            deps/libuv/src/unix/udp.c
            deps/libuv/src/unix/proctitle.c
    )

    if (ENABLE_SSL)
        find_package (OpenSSL)

        if (OPENSSL_FOUND)
            include_directories (${OPENSSL_INCLUDE_DIRS})
            set (LIBS ${LIBS} ${OPENSSL_LIBRARIES})
        endif ()
    endif (ENABLE_SSL)

    set (LIBS ${LIBS} )

    # TODO: Android etc., all the stuff we used to support before switching
    # to CMake.  libuv's gyp script is a good reference.
    #

    if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

        set (SOURCES ${SOURCES} 
                src/unix/eventqueue/kqueue.c
                deps/libuv/src/unix/kqueue.c
            )

        set (SOURCES ${SOURCES}
                deps/libuv/src/unix/darwin.c
                deps/libuv/src/unix/fsevents.c
                deps/libuv/src/unix/darwin-proctitle.c
            )

    elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")

        set (SOURCES ${SOURCES}
                src/unix/eventqueue/epoll.c
                deps/libuv/src/unix/linux-core.c
                deps/libuv/src/unix/linux-inotify.c
                deps/libuv/src/unix/linux-syscalls.c
            )

    else ()
        set (SOURCES ${SOURCES} src/unix/eventqueue/select.c)
    endif ()

    check_include_files (malloc.h HAVE_MALLOC_H)
    check_include_files (netdb.h HAVE_NETDB_H)
    check_include_files (sys/prctl.h HAVE_SYS_PRCTL_H)
    check_include_files (sys/sendfile.h HAVE_SYS_SENDFILE_H)
    check_include_files (sys/timerfd.h HAVE_SYS_TIMERFD_H)

    check_symbol_exists (PR_SET_NAME "prctl.h" HAVE_DECL_PR_SET_NAME)
    check_symbol_exists (TCP_CORK "netinet/tcp.h" HAVE_DECL_TCP_CORK)
    check_symbol_exists (TCP_NOPUSH "netinet/tcp.h" HAVE_DECL_TCP_NOPUSH)
    check_symbol_exists (MSG_NOSIGNAL "sys/socket.h" HAVE_DECL_MSG_NOSIGNAL)
    check_symbol_exists (SO_NOSIGPIPE "sys/socket.h" HAVE_DECL_SO_NOSIGPIPE)

    check_function_exists(kqueue USE_KQUEUE)

    if (NOT USE_KQUEUE)
        check_function_exists (epoll_create USE_EPOLL)
    endif ()

else ()
    message (FATAL_ERROR "Unknown platform (not Windows or UNIX?)")
endif ()

check_function_exists (timegm HAVE_TIMEGM)

add_library (lacewing ${SOURCES})

set_target_properties(lacewing PROPERTIES VERSION 0.5.4
                                          SOVERSION 0.5)

target_link_libraries (lacewing ${LIBS})

# MSVC only supports C89, so compile everything as C++ there
#
if (MSVC)
    set_source_files_properties(${SOURCES} PROPERTIES LANGUAGE CXX)
endif ()

configure_file ("${PROJECT_SOURCE_DIR}/config.h.in"
                "${PROJECT_BINARY_DIR}/config.h")

include_directories (${PROJECT_BINARY_DIR})

install(TARGETS lacewing DESTINATION lib)
install(FILES include/lacewing.h DESTINATION include)









