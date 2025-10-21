cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

if (POLICY CMP0054)
    cmake_policy(SET CMP0054 NEW)
endif()
if (POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
endif()
if (POLICY CMP0075)
    cmake_policy(SET CMP0075 NEW)
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release
            CACHE STRING "Set build type to Debug o Release (default Release)" FORCE)
endif()
string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_LOWER)

project(libevent C)

set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

list(APPEND CMAKE_MODULE_PATH "${SOURCE_DIR}/cmake/")

include(CheckTypeSize)
include(CheckFileOffsetBits)
include(Macros)
include(CheckVariableExists)
include(CheckSymbolExists)
include(CheckStructHasMember)
include(CheckCSourceCompiles)
include(CheckPrototypeDefinition)
include(CheckFunctionKeywords)
include(CheckConstExists)
include(AddCompilerFlags)

set(EVENT_VERSION_MAJOR ${EVENT_GIT___VERSION_MAJOR})
set(EVENT_VERSION_MINOR ${EVENT_GIT___VERSION_MINOR})
set(EVENT_VERSION_PATCH ${EVENT_GIT___VERSION_PATCH})
set(EVENT_VERSION_STAGE ${EVENT_GIT___VERSION_STAGE})


set(EVENT_ABI_MAJOR ${EVENT_VERSION_MAJOR})
set(EVENT_ABI_MINOR ${EVENT_VERSION_MINOR})
set(EVENT_ABI_PATCH ${EVENT_VERSION_PATCH})

set(EVENT_ABI_LIBVERSION
        "${EVENT_ABI_MAJOR}.${EVENT_ABI_MINOR}.${EVENT_ABI_PATCH}")

set(EVENT_PACKAGE_VERSION
        "${EVENT_VERSION_MAJOR}.${EVENT_VERSION_MINOR}.${EVENT_VERSION_PATCH}")

set(EVENT_NUMERIC_VERSION 0x02010c00)
# equals to VERSION_INFO in Makefile.am
set(EVENT_ABI_LIBVERSION_CURRENT   7)
set(EVENT_ABI_LIBVERSION_REVISION  1)
set(EVENT_ABI_LIBVERSION_AGE       0)

# equals to RELEASE in Makefile.am
set(EVENT_PACKAGE_RELEASE 2.1)

# only a subset of names can be used, defaults to "beta"
set(EVENT_STAGE_NAME ${EVENT_VERSION_STAGE})

set(EVENT_STAGE_NAME stable)

set(EVENT_VERSION
        "${EVENT_VERSION_MAJOR}.${EVENT_VERSION_MINOR}.${EVENT_VERSION_PATCH}-${EVENT_STAGE_NAME}")

# TODO: Add --disable-largefile     omit support for large files
option(EVENT__COVERAGE
        "Enable running gcov to get a test coverage report (only works with GCC/CLang). Make sure to enable -DCMAKE_BUILD_TYPE=Debug as well." OFF)

# Put the libaries and binaries that get built into directories at the
# top of the build tree rather than in hard-to-find leaf directories.
#
# But only if this variables are not defined yet
# (i.e. libevent is used via add_subdirectory())
if (NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
endif()
if (NOT DEFINED CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
endif()
if (NOT DEFINED CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
endif()

# make it colorful under ninja-build
if ("${CMAKE_GENERATOR}" STREQUAL "Ninja")
    add_compiler_flags(-fdiagnostics-color=always)
endif()

# Setup compiler flags for coverage.
if (EVENT__COVERAGE)
    if (NOT "${CMAKE_BUILD_TYPE_LOWER}" STREQUAL "debug")
        message(FATAL_ERROR "Coverage requires -DCMAKE_BUILD_TYPE=Debug")
    endif()

    message(STATUS "Setting coverage compiler flags")

    set(CMAKE_REQUIRED_LIBRARIES "--coverage")
    add_compiler_flags(-g -O0 --coverage)
    set(CMAKE_REQUIRED_LIBRARIES "")
endif()

set(GNUC 0)
set(CLANG 0)
set(MSVC 0)

if (("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang") OR
("${CMAKE_C_COMPILER_ID}" STREQUAL "AppleClang"))
    set(CLANG 1)
endif()
if (("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU") OR (${CLANG}))
    set(GNUC 1)
endif()

if ("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
    set(MSVC 1)
endif()


# Detect library type
set(EVENT_LIBRARY_TYPE "STATIC")

set(EVENT_LIBRARY_STATIC OFF)
set(EVENT_LIBRARY_SHARED OFF)
if ("${EVENT_LIBRARY_TYPE}" STREQUAL "STATIC")
    set(EVENT_LIBRARY_STATIC ON)
elseif ("${EVENT_LIBRARY_TYPE}" STREQUAL "SHARED")
    set(EVENT_LIBRARY_SHARED ON)
else()
    message(FATAL_ERROR "${EVENT_LIBRARY_TYPE} is not supported")
endif()

if (${MSVC})
    set(msvc_static_runtime OFF)
    if ("${EVENT_LIBRARY_TYPE}" STREQUAL "STATIC")
        set(msvc_static_runtime ON)
    endif()

    option(EVENT__MSVC_STATIC_RUNTIME
            "Link static runtime libraries"
            ${msvc_static_runtime})

    if (EVENT__MSVC_STATIC_RUNTIME)
        foreach (flag_var
                CMAKE_C_FLAGS_DEBUG
                CMAKE_C_FLAGS_RELEASE
                CMAKE_C_FLAGS_MINSIZEREL
                CMAKE_C_FLAGS_RELWITHDEBINFO
        )
            if (${flag_var} MATCHES "/MD")
                string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
            endif()
        endforeach()
    endif()
endif()

# GNUC specific options.
if (${GNUC})
    option(EVENT__DISABLE_GCC_WARNINGS "Disable verbose warnings with GCC" OFF)
    option(EVENT__ENABLE_GCC_HARDENING "Enable compiler security checks" OFF)
    option(EVENT__ENABLE_GCC_FUNCTION_SECTIONS "Enable gcc function sections" OFF)
    option(EVENT__ENABLE_GCC_WARNINGS "Make all GCC warnings into errors" OFF)

    set(GCC_V ${CMAKE_C_COMPILER_VERSION})

    list(APPEND __FLAGS
            -Wall -Wextra -Wno-unused-parameter -Wstrict-aliasing -Wstrict-prototypes

            -fno-strict-aliasing # gcc 2.9.5+
            -Wmissing-prototypes

            # gcc 4
            -Winit-self
            -Wmissing-field-initializers
            -Wdeclaration-after-statement

            # gcc 4.2
            -Waddress
            -Wnormalized=id
            -Woverride-init

            # gcc 4.5
            -Wlogical-op

            -Wwrite-strings
    )

    if (${CLANG})
        list(APPEND __FLAGS -Wno-unused-function)
    endif()

    if (EVENT__DISABLE_GCC_WARNINGS)
        list(APPEND __FLAGS -w)
    endif()

    if (EVENT__ENABLE_GCC_HARDENING)
        list(APPEND __FLAGS
                -fstack-protector-all
                -fwrapv
                -fPIE
                -Wstack-protector
                "--param ssp-buffer-size=1")

        add_definitions(-D_FORTIFY_SOURCE=2)
    endif()

    if (EVENT__ENABLE_GCC_FUNCTION_SECTIONS)
        list(APPEND __FLAGS -ffunction-sections)
        # TODO: Add --gc-sections support. We need some checks for NetBSD to ensure this works.
    endif()

    if (EVENT__ENABLE_GCC_WARNINGS)
        list(APPEND __FLAGS -Werror)
    endif()

    add_compiler_flags(${__FLAGS})
endif()

if (APPLE)
    # Get rid of deprecated warnings for OpenSSL on OSX 10.7 and greater.
    add_compiler_flags(
            -Wno-error=deprecated-declarations
            -Qunused-arguments
    )
endif()

if (MINGW OR CYGWIN)
    set(WIN32 TRUE)
endif()

# Winsock.
if(WIN32)
    set(CMAKE_REQUIRED_LIBRARIES  ws2_32 shell32 advapi32)
    set(CMAKE_REQUIRED_DEFINITIONS -FIwinsock2.h -FIws2tcpip.h -D_WIN32_WINNT=0x0600)
endif()

# Check if _GNU_SOURCE is available.
if (NOT DEFINED _GNU_SOURCE)
    CHECK_SYMBOL_EXISTS(__GNU_LIBRARY__ "features.h" _GNU_SOURCE)

    if (NOT _GNU_SOURCE)
        unset(_GNU_SOURCE CACHE)
        CHECK_SYMBOL_EXISTS(_GNU_SOURCE "features.h" _GNU_SOURCE)
    endif()

    if (ANDROID)
        set(_GNU_SOURCE TRUE)
    endif()
endif()

if (_GNU_SOURCE)
    add_definitions(-D_GNU_SOURCE=1)
    set(CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
endif()

list(APPEND EVENT_INCLUDES
        fcntl.h
        inttypes.h
        memory.h
        signal.h
        stdarg.h
        stddef.h
        stdint.h
        stdlib.h
        string.h
        errno.h
        time.h
        sys/types.h
        sys/stat.h
)

if (WIN32)
    list(APPEND EVENT_INCLUDES
            io.h
            winsock2.h
            ws2tcpip.h
            afunix.h
    )
else()
    list(APPEND EVENT_INCLUDES
        unistd.h
        sys/time.h
        sys/param.h
        netdb.h
        dlfcn.h
        arpa/inet.h
        poll.h
        sys/socket.h
        sys/random.h
        sys/un.h
        sys/ioctl.h
        sys/mman.h
        sys/queue.h
        sys/select.h
        sys/uio.h
        sys/wait.h
        sys/resource.h
        netinet/in.h
        netinet/tcp.h
        ifaddrs.h
    )
    if (APPLE)
        list(APPEND EVENT_INCLUDES
            sys/event.h
            mach/mach_time.h
            mach/mach.h
            sys/sysctl.h
        )
    elseif (ANDROID)
        list(APPEND EVENT_INCLUDES
            sys/epoll.h
            sys/eventfd.h
            sys/sendfile.h
            sys/timerfd.h
            netinet/in6.h
        )
    endif ()
endif ()

foreach(FILE ${EVENT_INCLUDES})
    string(REGEX REPLACE "[./]" "_" FILE_UL ${FILE})
    string(TOUPPER "${FILE_UL}" FILE_UL_UPPER)
    set("EVENT__HAVE_${FILE_UL_UPPER}" 1)
endforeach()

set(EVENT__HAVE_GETADDRINFO 1)
set(EVENT__HAVE_GETPROTOBYNUMBER 1)
set(EVENT__HAVE_GETSERVBYNAME 1)
set(EVENT__HAVE_INET_PTON 1)
set(EVENT__HAVE_SIGNAL 1)
set(EVENT__HAVE_STRTOLL 1)
set(EVENT__HAVE_TIMERCLEAR 1)
set(EVENT__HAVE_TIMERISSET 1)
set(EVENT__HAVE_TIMERCMP 1)
set(EVENT__HAVE_PUTENV 1)
set(EVENT__HAVE_GETNAMEINFO 1)
set(EVENT__HAVE___func__ 1)
set(EVENT__HAVE___FUNCTION__ 1)
# typesize
set(EVENT__HAVE_STRUCT_SOCKADDR_UN 1)
set(EVENT__HAVE_STRUCT_SOCKADDR_IN6 1)
set(EVENT__HAVE_STRUCT_SOCKADDR_STORAGE 1)
set(EVENT__HAVE_STRUCT_SOCKADDR_STORAGE_SS_FAMILY 1)
set(EVENT__HAVE_STRUCT_IN6_ADDR 1)
set(EVENT__HAVE_UINT8_T 1)
set(EVENT__HAVE_UINT16_T 1)
set(EVENT__HAVE_UINT32_T 1)
set(EVENT__HAVE_UINT64_T 1)
set(EVENT__HAVE_UINTPTR_T 1)
set(EVENT__HAVE_STRUCT_ADDRINFO 1)
set(EVENT__HAVE_STRUCT_LINGER 1)

set(EVENT__SIZEOF_SHORT 2)
set(EVENT__SIZEOF_INT 4)
set(EVENT__SIZEOF_LONG_LONG 8)
set(EVENT__SIZEOF_SIZE_T 8)
set(EVENT__SIZEOF_SSIZE_T 8)
set(EVENT__SIZEOF_SOCKLEN_T 4)
set(EVENT__SIZEOF_VOID_P 8)

set(EVENT__inline inline)
set(EVENT__size_t size_t)
set(EVENT__socklen_t socklen_t)

set(EVENT__HAVE_DECL_CTL_KERN 0)
set(EVENT__HAVE_DECL_KERN_ARND 0)
if (WIN32)
    set(EVENT__HAVE_UMASK 1)
    set(EVENT__HAVE__GMTIME64_S 1)
    set(EVENT__HAVE__GMTIME64 1)

    set(EVENT__SIZEOF_LONG 4)
    set(EVENT__SIZEOF_OFF_T 4)

    set(EVENT__ssize_t SSIZE_T)
else ()
    set(EVENT__HAVE_INET_NTOP 1)
    set(EVENT__HAVE_GETTIMEOFDAY 1)
    set(EVENT__HAVE_STRLCPY 1)
    set(EVENT__HAVE_STRSEP 1)
    set(EVENT__HAVE_STRTOK_R 1)
    set(EVENT__HAVE_VASPRINTF 1)
    set(EVENT__HAVE_TIMERADD 1)
    set(EVENT__HAVE_NANOSLEEP 1)
    set(EVENT__HAVE_CLOCK_GETTIME 1)
    set(EVENT__HAVE_GETIFADDRS 1)
    set(EVENT__HAVE_SELECT 1)
    set(EVENT__HAVE_POLL 1)
    set(EVENT__HAVE_FCNTL 1)
    set(EVENT__HAVE_MMAP 1)
    set(EVENT__HAVE_PIPE 1)
    set(EVENT__HAVE_SENDFILE 1)
    set(EVENT__HAVE_SIGACTION 1)
    set(EVENT__HAVE_ARC4RANDOM 1)
    set(EVENT__HAVE_ARC4RANDOM_BUF 1)
    set(EVENT__HAVE_GETEGID 1)
    set(EVENT__HAVE_GETEUID 1)
    set(EVENT__HAVE_USLEEP 1)
    set(EVENT__HAVE_SETENV 1)
    set(EVENT__HAVE_UNSETENV 1)
    set(EVENT__HAVE_SETRLIMIT 1)
    set(EVENT__HAVE_TAILQFOREACH 1)
    set(EVENT__HAVE_SETFD 1)
    set(EVENT__HAVE_FD_MASK 1)
    set(EVENT__DNS_USE_CPU_CLOCK_FOR_ID 1)
    set(EVENT__HAVE_SA_FAMILY_T 1)

    set(EVENT__SIZEOF_LONG 8)
    set(EVENT__SIZEOF_OFF_T 8)
    set(EVENT__SIZEOF_PTHREAD_T 8)

    set(EVENT__ssize_t ssize_t)
    if (ANDROID)
        set(EVENT__HAVE_SPLICE 1)
        set(EVENT__HAVE_ACCEPT4 1)
        set(EVENT__HAVE_GETRANDOM 1)
        set(EVENT__HAVE_EPOLL_CREATE1 1)
        set(EVENT__HAVE_EPOLL_CTL 1)
        set(EVENT__HAVE_EPOLL 1)
        set(EVENT__HAVE_GETHOSTBYNAME_R 1)
        set(EVENT__HAVE_GETHOSTBYNAME_R_6_ARG 1)
        # set(EVENT__HAVE_PIPE2 1)
        set(EVENT__HAVE_TIMERFD_CREATE 1)
        set(EVENT__HAVE_EVENTFD 1)

        set(EVENT__HAVE_STRUCT_IN6_ADDR_S6_ADDR16 1)
        set(EVENT__HAVE_STRUCT_IN6_ADDR_S6_ADDR32 1)
    elseif (APPLE)
        set(EVENT__HAVE_UMASK 1)
        set(EVENT__HAVE_ARC4RANDOM_ADDRANDOM 1)
        set(EVENT__HAVE_ISSETUGID 1)
        set(EVENT__HAVE_KQUEUE 1)
        set(EVENT__HAVE_MACH_ABSOLUTE_TIME 1)
        set(EVENT__HAVE_SYSCTL 1)

        set(EVENT__HAVE_DECL_CTL_KERN 1)
        set(EVENT__HAVE_STRUCT_SOCKADDR_IN6_SIN6_LEN 1)
    endif ()
endif ()

# Only `CHECK_TYPE_SIZE()' will use `CMAKE_EXTRA_INCLUDE_FILES'
set(CMAKE_EXTRA_INCLUDE_FILES ${EVENT_INCLUDES})

# Winssck.
if (_MSC_VER)
    list(APPEND CMAKE_EXTRA_INCLUDE_FILES BaseTsd.h)
endif()

if (NOT WIN32)
    list(APPEND CMAKE_EXTRA_INCLUDE_FILES pthread.h)
endif()


if(EVENT__HAVE_NETDB_H)
    list(APPEND CMAKE_EXTRA_INCLUDE_FILES netdb.h)
elseif(WIN32)
    list(APPEND CMAKE_EXTRA_INCLUDE_FILES ws2tcpip.h)
endif()

# Check for sockaddr structure sizes.
set(SOCKADDR_HEADERS)
if (WIN32)
    set(CMAKE_REQUIRED_DEFINITIONS "-DWIN32_LEAN_AND_MEAN")
    if (_MSC_VER LESS 1300)
        set(SOCKADDR_HEADERS winsock.h)
    else()
        set(SOCKADDR_HEADERS winsock2.h ws2tcpip.h)
    endif()
else()
    if (EVENT__HAVE_NETINET_IN_H)
        set(SOCKADDR_HEADERS ${SOCKADDR_HEADERS} netinet/in.h)
    endif()

    if (EVENT__HAVE_NETINET_IN6_H)
        set(SOCKADDR_HEADERS ${SOCKADDR_HEADERS} netinet/in6.h)
    endif()

    if (EVENT__HAVE_SYS_SOCKET_H)
        set(SOCKADDR_HEADERS ${SOCKADDR_HEADERS} sys/socket.h)
    endif()

    if (EVENT__HAVE_NETDB_H)
        set(SOCKADDR_HEADERS ${SOCKADDR_HEADERS} netdb.h)
    endif()
endif()


# Group the source files.
set(HDR_PRIVATE
        ${SOURCE_DIR}/bufferevent-internal.h
        ${SOURCE_DIR}/changelist-internal.h
        ${SOURCE_DIR}/defer-internal.h
        ${SOURCE_DIR}/epolltable-internal.h
        ${SOURCE_DIR}/evbuffer-internal.h
        ${SOURCE_DIR}/event-internal.h
        ${SOURCE_DIR}/evmap-internal.h
        ${SOURCE_DIR}/evrpc-internal.h
        ${SOURCE_DIR}/evsignal-internal.h
        ${SOURCE_DIR}/evthread-internal.h
        ${SOURCE_DIR}/ht-internal.h
        ${SOURCE_DIR}/http-internal.h
        ${SOURCE_DIR}/iocp-internal.h
        ${SOURCE_DIR}/ipv6-internal.h
        ${SOURCE_DIR}/log-internal.h
        ${SOURCE_DIR}/minheap-internal.h
        ${SOURCE_DIR}/mm-internal.h
        ${SOURCE_DIR}/ratelim-internal.h
        ${SOURCE_DIR}/strlcpy-internal.h
        ${SOURCE_DIR}/util-internal.h
        ${SOURCE_DIR}/evconfig-private.h
        ${SOURCE_DIR}/compat/sys/queue.h)

set(HDR_COMPAT
        ${SOURCE_DIR}/include/evdns.h
        ${SOURCE_DIR}/include/evrpc.h
        ${SOURCE_DIR}/include/event.h
        ${SOURCE_DIR}/include/evhttp.h
        ${SOURCE_DIR}/include/evutil.h)

set(HDR_PUBLIC
        ${SOURCE_DIR}/include/event2/buffer.h
        ${SOURCE_DIR}/include/event2/bufferevent.h
        ${SOURCE_DIR}/include/event2/bufferevent_compat.h
        ${SOURCE_DIR}/include/event2/bufferevent_struct.h
        ${SOURCE_DIR}/include/event2/buffer_compat.h
        ${SOURCE_DIR}/include/event2/dns.h
        ${SOURCE_DIR}/include/event2/dns_compat.h
        ${SOURCE_DIR}/include/event2/dns_struct.h
        ${SOURCE_DIR}/include/event2/event.h
        ${SOURCE_DIR}/include/event2/event_compat.h
        ${SOURCE_DIR}/include/event2/event_struct.h
        ${SOURCE_DIR}/include/event2/http.h
        ${SOURCE_DIR}/include/event2/http_compat.h
        ${SOURCE_DIR}/include/event2/http_struct.h
        ${SOURCE_DIR}/include/event2/keyvalq_struct.h
        ${SOURCE_DIR}/include/event2/listener.h
        ${SOURCE_DIR}/include/event2/rpc.h
        ${SOURCE_DIR}/include/event2/rpc_compat.h
        ${SOURCE_DIR}/include/event2/rpc_struct.h
        ${SOURCE_DIR}/include/event2/tag.h
        ${SOURCE_DIR}/include/event2/tag_compat.h
        ${SOURCE_DIR}/include/event2/thread.h
        ${SOURCE_DIR}/include/event2/util.h
        ${SOURCE_DIR}/include/event2/visibility.h
        ${PROJECT_BINARY_DIR}/include/event2/event-config.h)

set(SRC_CORE
        ${SOURCE_DIR}/buffer.c
        ${SOURCE_DIR}/bufferevent.c
        ${SOURCE_DIR}/bufferevent_filter.c
        ${SOURCE_DIR}/bufferevent_pair.c
        ${SOURCE_DIR}/bufferevent_ratelim.c
        ${SOURCE_DIR}/bufferevent_sock.c
        ${SOURCE_DIR}/event.c
        ${SOURCE_DIR}/evmap.c
        ${SOURCE_DIR}/evthread.c
        ${SOURCE_DIR}/evutil.c
        ${SOURCE_DIR}/evutil_rand.c
        ${SOURCE_DIR}/evutil_time.c
        ${SOURCE_DIR}/listener.c
        ${SOURCE_DIR}/log.c
        ${SOURCE_DIR}/signal.c
        ${SOURCE_DIR}/strlcpy.c)

if(EVENT__HAVE_SELECT)
    list(APPEND SRC_CORE ${SOURCE_DIR}/select.c)
endif()

if(EVENT__HAVE_POLL)
    list(APPEND SRC_CORE ${SOURCE_DIR}/poll.c)
endif()

if(EVENT__HAVE_EPOLL)
    list(APPEND SRC_CORE ${SOURCE_DIR}/epoll.c)
endif()

if (WIN32)
    list(APPEND SRC_CORE ${SOURCE_DIR}/evthread_win32.c)
else()
    find_package(Threads REQUIRED)
    if (NOT CMAKE_USE_PTHREADS_INIT)
        message(FATAL_ERROR "Failed to find Pthreads")
    endif()

    set(EVENT__HAVE_PTHREADS 1)
    list(APPEND LIB_APPS ${CMAKE_THREAD_LIBS_INIT})
endif()

set(SRC_EXTRA
        ${SOURCE_DIR}/event_tagging.c
        ${SOURCE_DIR}/http.c
        ${SOURCE_DIR}/evdns.c
        ${SOURCE_DIR}/evrpc.c)

add_definitions(-DHAVE_CONFIG_H)

# We use BEFORE here so we don't accidentally look in system directories
# first for some previous versions of the headers that are installed.
include_directories(${SOURCE_DIR}
        ${SOURCE_DIR}/compat
        ${SOURCE_DIR}/include)

if(WIN32)
    list(APPEND SRC_CORE
            ${SOURCE_DIR}/buffer_iocp.c
            ${SOURCE_DIR}/bufferevent_async.c
            ${SOURCE_DIR}/event_iocp.c
            ${SOURCE_DIR}/win32select.c)

    list(APPEND HDR_PRIVATE ${SOURCE_DIR}/WIN32-Code/getopt.h)

    set(EVENT__DNS_USE_FTIME_FOR_ID 1)
    set(LIB_PLATFORM ws2_32 shell32 advapi32)
    add_definitions(
            -D_CRT_SECURE_NO_WARNINGS
            -D_CRT_NONSTDC_NO_DEPRECATE)

    include_directories(${SOURCE_DIR}/WIN32-Code)
endif()

source_group("Headers Private"  FILES ${HDR_PRIVATE})
source_group("Header Compat"    FILES ${HDR_COMPAT})
source_group("Headers Public"   FILES ${HDR_PUBLIC})
source_group("Source Core"      FILES ${SRC_CORE})
source_group("Source Extra"     FILES ${SRC_EXTRA})

# Generate the configure headers.
# (Place them in the build dir so we don't polute the source tree with generated files).
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

if (${GNUC})
    set(EVENT_SHARED_FLAGS -fvisibility=hidden)
elseif ("${CMAKE_C_COMPILER_ID}" STREQUAL "SunPro")
    set(EVENT_SHARED_FLAGS -xldscope=hidden)
endif()

configure_file(
        ${SOURCE_DIR}/event-config.h.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/include/event2/event-config.h
        NEWLINE_STYLE UNIX)

configure_file(
        ${SOURCE_DIR}/evconfig-private.h.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/include/evconfig-private.h)

#
# Create the libraries.
#
include(AddEventLibrary)
add_event_library(event_core SOURCES ${SRC_CORE})

add_event_library(event SOURCES ${SRC_CORE} ${SRC_EXTRA})

if (EVENT__HAVE_PTHREADS)
    set(SRC_PTHREADS ${SOURCE_DIR}/evthread_pthread.c)
    add_event_library(event_pthreads
            INNER_LIBRARIES event_core
            SOURCES ${SRC_PTHREADS})
    target_link_libraries(event
            event_pthreads)
endif()

set(WIN32_GETOPT)
if (WIN32)
    set(_TMPLIBS)
    if (${EVENT_LIBRARY_STATIC})
        list(APPEND _TMPLIBS event_core event)
    endif()
    if (${EVENT_LIBRARY_SHARED})
        list(APPEND _TMPLIBS event_core event)
    endif()
    foreach(lib ${_TMPLIBS})
        target_link_libraries(${lib} iphlpapi)
    endforeach()
    unset(_TMPLIBS)

    list(APPEND WIN32_GETOPT
            WIN32-Code/getopt.c
            WIN32-Code/getopt_long.c)
endif()

# Generate the config file for the build-tree.
set(EVENT__INCLUDE_DIRS
        "${PROJECT_DIR}/include"
        "${PROJECT_BINARY_DIR}/include")

set(LIBEVENT_INCLUDE_DIRS
        ${EVENT__INCLUDE_DIRS}
        CACHE PATH "Libevent include directories")

# Generate version info for both build-tree and install-tree.
configure_file(${SOURCE_DIR}/cmake/LibeventConfigVersion.cmake.in
        ${PROJECT_BINARY_DIR}/LibeventConfigVersion.cmake
        @ONLY)

target_include_directories(event
        INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_BINARY_DIR}/include
)