liblacewing is a library for writing cross-platform, networked applications in C/C++.

http://lacewing-project.org/

http://github.com/udp/lacewing/

[![Build Status](https://secure.travis-ci.org/udp/lacewing.png?branch=master)](http://travis-ci.org/udp/lacewing)


Installing
==========

Linux, OS X:

    ./configure
    make
    make install

FreeBSD (GNU make required):

    ./configure
    gmake
    gmake install

Windows (MinGW/MSYS only):

    ./configure --disable-spdy
    make

An MSVC project file is available in the `msvc` directory.


Documentation
=============

The API documentation for liblacewing can be found [here](http://lacewing-project.org/docs).

Issues with the documentation can be reported in the [gh-pages branch](https://github.com/udp/lacewing/tree/gh-pages)
of the liblacewing GitHub repository.


Changes in 0.5.4 (2013-Oct-31)
==============================

- `post_eventloop_exit` now works correctly on UNIX

- Socket address is now stored and retrievable for server/webserver clients

- Suppressed warnings about unused `lwp_release` function when compiling the library

- Fix issue with the webserver accessing freed pointers when writing response headers

[0.5.4 Release Notes](http://lacewing-project.org/release/0.5.4.html)



Changes in 0.5.3 (2013-Aug-02)
==============================

- `lw_eventpump_start_sleepy_ticking` is now supported on UNIX

- `lw_event` now supports infinite timeouts

- C++ wrapper for `pump::add` now returns the `lw_pump_watch`

- The makefile now respects autoconf `libdir` and `includedir`

- The makefile now produces an unversioned dynamic library, while still installing with the version suffix

[0.5.3 Release Notes](http://lacewing-project.org/release/0.5.3.html)


Changes in 0.5.2 (2013-Jun-05)
==============================

- The library can now build on Unix systems where neither `TCP_CORK` nor `TCP_NOPUSH` are available

- The library can now build on systems without `AI_V4MAPPED` and `AI_ADDRCONFIG`

- `alloca.h` is no longer required for SPDY support

- Tags added to `lw_addr`, `lw_thread`, `lw_filter`, `lw_pump` and `lw_client`

- MSVC static builds no longer use `__declspec(dllexport)`

- Windows: `shutdown` is now called on sockets before `closesocket`

- C++ API: Tag getters/setters added to all classes

- C++ API: Server connect and disconnect hooks changed to receive a `server_client` instead of a `client`

- Windows: Fixed various 0.5.0 regressions in `lw_client`

- Unix: An `EINTR` result from `epoll_wait` or `kqueue` no longer causes `lw_eventpump_start_eventloop` to terminate

- 0.5.0 regressions with `lw_server` refcounting fixed, which prevented the client list from functioning

- Webserver: Issue with `on_upload_post` hook being fired multiple times fixed

[0.5.2 Release Notes](http://lacewing-project.org/release/0.5.2.html)


Changes in 0.5.1 (2013-Mar-22)
==============================

- Fix `lw_addr_clone` segfault

- `outer_size` renamed to `tail_size`, new `lw_stream_from_tail` function added

- Tag added to `lw_udp` and `lw_flashpolicy`

- Prototype for `lw_filter_clone` was erroneously called `lw_filter_copy` 

- Various fixes for `lw_udp`

- New `lw_addr_type` and `lw_addr_set_type` functions added

- `lw_udp_host_addr` now respects address IPv6 setting

- Fix problems with the Unix implementation of `lw_timer_stop`

- Fix segfault with Windows FDStream when the completion routine fires after deletion

- Fix some problems building on 64-bit Windows

[0.5.1 Release Notes](http://lacewing-project.org/release/0.5.1.html)


Changes in 0.5.0 (2013-Mar-11)
==============================

- Library ported to C99

- Lots of API changes for consistency/correctness

- C++ API converted to `snake_case` to be consistent with the C API

- UNIX `EventPump` will now exit the event loop if epoll or kqueue throws an error

- The various types of internal ref counting have been replaced with a more consistent `lwp_retain`/`lwp_release`

- Preliminary support for [SWIG](http://swig.org/)

- Updated SPDY and [http-parser](https://github.com/joyent/http-parser) snapshots

- `write(3)` is now used for non-sockets when `SO_NOSIGPIPE` is not available

- Various fixes with webserver sessions

- New `ENABLE_SSL` option in Makefile

[0.5.0 Release Notes](http://lacewing-project.org/release/0.5.0.html)


Changes in 0.4.3 (2012-Oct-28)
==============================

- Various issues with the Windows implementation of `FDStream` resolved

- Visual Studio projects added for examples

- Issues with building under MinGW resolved

- `Server::Client::NPN` stub added for Windows

- New parameter to `Stream::Close` to allow cancellation of pending writes

- Public header file renamed from `Lacewing.h` to `lacewing.h` (lowercase)

- C webserver examples updated for the current C API

- Updated to the latest version of [multipart-parser-c](https://github.com/iafonov/multipart-parser-c)

[0.4.3 Release Notes](http://lacewing-project.org/release/0.4.3.html)


Changes in 0.4.2 (2012-Aug-23)
==============================

- Fix issue with POST data truncation (0.4.1 regression)

- Cookies were not correctly parsed

- StreamGraph did not read beyond any filters present in a graph

[0.4.2 Release Notes](http://lacewing-project.org/release/0.4.2.html)


Changes in 0.4.1 (2012-Aug-22)
==============================

- Fix issue with HTTP data not always being completely processed

- Fix error with URL decoding function which lead to trailing garbage in decoded strings 

[0.4.1 Release Notes](http://lacewing-project.org/release/0.4.1.html)


Changes in 0.4.0 (2012-Aug-21)
==============================

- `Webserver` now supports the SPDY protocol, and much of the HTTP code has been improved

- Resolve various issues with the stream system

- New option in `FDStream::SetFD` to close the FD on destruction, which is now used by `File`

- If `Stream::WriteFile` failed to open a file, the stream would be permanently blocked

- `Server` now supports TLS NPN where available

- The code for multipart POST data handling has been replaced with [multipart-parser-c](https://github.com/iafonov/multipart-parser-c)

- Fixed IPv4 support in `Client`

- `File::OpenTemp` used incorrect filenames on Windows 

[0.4.0 Release Notes](http://lacewing-project.org/release/0.4.0.html)


Changes in 0.3.1 (2012-Jun-29)
=============================

- New Stream::WritePartial function to opt out of automatic buffering

- Fix bad length param in GetSockAddr function (causing problems with `::Port` etc)


Changes in 0.3.0 (2012-Jun-20)
==============================

- I/O is now separated into classes such as `Stream` and `FDStream` instead of being provided separately by each class.

- The Relay* classes have been removed (they may become a separate library later)

- HTTP parser switched out for joyent/http-parser

- IPv6 is now supported

- Lots of miscellaneous fixes/refactoring


Changes in 0.2.6 (2011-Dec-18)
==============================

- `Webserver::Request::Cookie` now has a parameter allowing you to set attributes for each cookie.  By default,
  both the `Secure` and `HttpOnly` attributes are set.

- You can now iterate through the headers, cookies, GET/POST parameters and session data stored in a `Webserver::Request`

- Bug when parsing fragmented HTTP requests fixed.  This made `Webserver` incompatible with recent fixes for the SSL
  BEAST attack.

- The library now builds as `liblacewing.dylib` on OS X instead of `liblacewing.so`

- The source should now build using MinGW (although the included makefile will not work on Windows)


Changes in 0.2.5 (2011-Oct-11)
==============================

- Resolve various issues with the build system


Changes in 0.2.4 (2011-Oct-01)
==============================

- Some functions were missing `LacewingFunction` in Lacewing.h, and thus were not correctly
  exported from the shared object/DLL :-

    * RelayServer::Channel::Next
    * RelayServer::Client::Next
    * RelayServer::ChannelCount
    * RelayServer::SetChannelListing

- The name of a client was set *before* calling the `onNameSet` handler in RelayServer

- Client disconnect issues on BSD/OS X fixed in Server

- `MSG_NOSIGNAL` or `SO_NOSIGPIPE` are now used everywhere, to prevent the generation of
  unwanted SIGPIPE signals on Unix

- Possible EventPump deadlock fixed (Unix)

- New functions introduced:

    * Client::CheapBuffering
    * UDP::Hosting

- FlashPlayerPolicy class renamed to FlashPolicy

- Miscellaneous fixes in the C API 


Changes in 0.2.3 (2011-Sep-07)
==============================

- Fixed an issue with Lacewing::EventPump on Windows, which would cause functions queued with
  Post() not to execute in some cases.  This lead to to a significant issue with client disconnects
  not being detected properly in Lacewing::Server.
  
- Miscellaneous relay protocol fixes

    
Changes in 0.2.2 (2011-Aug-31)
==============================

- Lacewing::Timer now ends the timer thread properly in the destructor.  In previous
  versions it was necessary to call Stop() explicitly to avoid a crash.
  
- Two issues with the EventPump class fixed:

    * StartEventLoop() could return prematurely in some cases (Unix only)
    * Possible segmentation fault when a null WriteCallback was specified

- Added the RelayClient and RelayServer source files to the Makefile and Android.mk

- SHA1_Hex no longer cuts off the last four bytes

- Updated relay protocol specification and implementation classes (now revision 3)

- Fixed issue with the GuessMimeType function returning application/octet-stream even
  for known file extensions

- Refactored the internals of Lacewing::Webserver for future SPDY support.  The HTTP
  specific code is now separate from the webserver itself.


Changes in 0.2.1 (2011-Jul-15)
==============================

- Fixed crash in the List utility class introduced by 0.2.0


Changes in 0.2.0 (2011-Jul-14)
==============================

- New Javascript liblacewing API for MozJS (SpiderMonkey/TraceMonkey) and V8.
  Currently only supports the webserver and utility classes.  It is also
  now possible to build liblacewing as a Node.JS module - a wscript for node-waf
  is provided.
  
- Added support for building for Android (Android.mk is in the 'jni' folder)

- The build system has been changed from automake to autoconf with a simple
  Makefile.  This should solve the headaches with building on different
  platforms (Windows users should still use the MSVC project as normal.) 

- Removed the dependency on STL, resulting in a much faster build and smaller
  binaries (particularly on Android.)  This also makes it easier to link
  liblacewing into static Linux binaries.

- New global hash functions, implemented via OpenSSL or the Win32 crypt32 API:

    * Lacewing::MD5
    * Lacewing::MD5_Hex
    * Lacewing::SHA1
    * Lacewing::SHA1_Hex
    
- PostEventLoopExit() function added to the EventPump, to cause StartEventLoop()
  to return (once any pending events have been processed.)

- In the Webserver, Request objects are now freed immediately after the
  connection is closed, after calling the disconnect handler.  Previously, the
  Request object would remain in a disconnected state until Finish() was called.

- In Unix, the EventPump class is is now just a default implementation of the
  Pump class, which can be derived from to use custom methods of watching FDs.
  A custom pump using libev is provided in the 'ev' folder.

- Fixed some issues with Lacewing::Server on Unix:

    * SendFile didn't always complete in SSL mode
    * SendFile didn't send the file if open() returned 0
    * The Disconnect handler didn't always trigger

    
