//
// detail/old_win_sdk_compat.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_OLD_WIN_SDK_COMPAT_HPP
#define BOOST_ASIO_DETAIL_OLD_WIN_SDK_COMPAT_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if defined(BOOST_WINDOWS) || defined(__CYGWIN__)

// Guess whether we are building against on old Platform SDK.
#if !defined(IN6ADDR_ANY_INIT)
#define BOOST_ASIO_HAS_OLD_WIN_SDK 1
#endif // !defined(IN6ADDR_ANY_INIT)

#if defined(BOOST_ASIO_HAS_OLD_WIN_SDK)

// Emulation of types that are missing from old Platform SDKs.
//
// N.B. this emulation is also used if building for a Windows 2000 target with
// a recent (i.e. Vista or later) SDK, as the SDK does not provide IPv6 support
// in that case.

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

enum
{
  sockaddr_storage_maxsize = 128, // Maximum size.
  sockaddr_storage_alignsize = (sizeof(__int64)), // Desired alignment.
  sockaddr_storage_pad1size = (sockaddr_storage_alignsize - sizeof(short)),
  sockaddr_storage_pad2size = (sockaddr_storage_maxsize -
      (sizeof(short) + sockaddr_storage_pad1size + sockaddr_storage_alignsize))
};

struct sockaddr_storage_emulation
{
  short ss_family;
  char __ss_pad1[sockaddr_storage_pad1size];
  __int64 __ss_align;
  char __ss_pad2[sockaddr_storage_pad2size];
};

struct in6_addr_emulation
{
  union
  {
    u_char Byte[16];
    u_short Word[8];
  } u;
};

#if !defined(s6_addr)
# define _S6_un u
# define _S6_u8 Byte
# define s6_addr _S6_un._S6_u8
#endif // !defined(s6_addr)

struct sockaddr_in6_emulation
{
  short sin6_family;
  u_short sin6_port;
  u_long sin6_flowinfo;
  in6_addr_emulation sin6_addr;
  u_long sin6_scope_id;
};

struct ipv6_mreq_emulation
{
  in6_addr_emulation ipv6mr_multiaddr;
  unsigned int ipv6mr_interface;
};

#if !defined(IN6ADDR_ANY_INIT)
# define IN6ADDR_ANY_INIT { 0 }
#endif

#if !defined(IN6ADDR_LOOPBACK_INIT)
# define IN6ADDR_LOOPBACK_INIT { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 }
#endif

struct addrinfo_emulation
{
  int ai_flags;
  int ai_family;
  int ai_socktype;
  int ai_protocol;
  size_t ai_addrlen;
  char* ai_canonname;
  sockaddr* ai_addr;
  addrinfo_emulation* ai_next;
};

#if !defined(AI_PASSIVE)
# define AI_PASSIVE 0x1
#endif

#if !defined(AI_CANONNAME)
# define AI_CANONNAME 0x2
#endif

#if !defined(AI_NUMERICHOST)
# define AI_NUMERICHOST 0x4
#endif

#if !defined(EAI_AGAIN)
# define EAI_AGAIN WSATRY_AGAIN
#endif

#if !defined(EAI_BADFLAGS)
# define EAI_BADFLAGS WSAEINVAL
#endif

#if !defined(EAI_FAIL)
# define EAI_FAIL WSANO_RECOVERY
#endif

#if !defined(EAI_FAMILY)
# define EAI_FAMILY WSAEAFNOSUPPORT
#endif

#if !defined(EAI_MEMORY)
# define EAI_MEMORY WSA_NOT_ENOUGH_MEMORY
#endif

#if !defined(EAI_NODATA)
# define EAI_NODATA WSANO_DATA
#endif

#if !defined(EAI_NONAME)
# define EAI_NONAME WSAHOST_NOT_FOUND
#endif

#if !defined(EAI_SERVICE)
# define EAI_SERVICE WSATYPE_NOT_FOUND
#endif

#if !defined(EAI_SOCKTYPE)
# define EAI_SOCKTYPE WSAESOCKTNOSUPPORT
#endif

#if !defined(NI_NOFQDN)
# define NI_NOFQDN 0x01
#endif

#if !defined(NI_NUMERICHOST)
# define NI_NUMERICHOST 0x02
#endif

#if !defined(NI_NAMEREQD)
# define NI_NAMEREQD 0x04
#endif

#if !defined(NI_NUMERICSERV)
# define NI_NUMERICSERV 0x08
#endif

#if !defined(NI_DGRAM)
# define NI_DGRAM 0x10
#endif

#if !defined(IPPROTO_IPV6)
# define IPPROTO_IPV6 41
#endif

#if !defined(IPV6_UNICAST_HOPS)
# define IPV6_UNICAST_HOPS 4
#endif

#if !defined(IPV6_MULTICAST_IF)
# define IPV6_MULTICAST_IF 9
#endif

#if !defined(IPV6_MULTICAST_HOPS)
# define IPV6_MULTICAST_HOPS 10
#endif

#if !defined(IPV6_MULTICAST_LOOP)
# define IPV6_MULTICAST_LOOP 11
#endif

#if !defined(IPV6_JOIN_GROUP)
# define IPV6_JOIN_GROUP 12
#endif

#if !defined(IPV6_LEAVE_GROUP)
# define IPV6_LEAVE_GROUP 13
#endif

inline int IN6_IS_ADDR_UNSPECIFIED(const in6_addr_emulation* a)
{
  return ((a->s6_addr[0] == 0)
      && (a->s6_addr[1] == 0)
      && (a->s6_addr[2] == 0)
      && (a->s6_addr[3] == 0)
      && (a->s6_addr[4] == 0)
      && (a->s6_addr[5] == 0)
      && (a->s6_addr[6] == 0)
      && (a->s6_addr[7] == 0)
      && (a->s6_addr[8] == 0)
      && (a->s6_addr[9] == 0)
      && (a->s6_addr[10] == 0)
      && (a->s6_addr[11] == 0)
      && (a->s6_addr[12] == 0)
      && (a->s6_addr[13] == 0)
      && (a->s6_addr[14] == 0)
      && (a->s6_addr[15] == 0));
}

inline int IN6_IS_ADDR_LOOPBACK(const in6_addr_emulation* a)
{
  return ((a->s6_addr[0] == 0)
      && (a->s6_addr[1] == 0)
      && (a->s6_addr[2] == 0)
      && (a->s6_addr[3] == 0)
      && (a->s6_addr[4] == 0)
      && (a->s6_addr[5] == 0)
      && (a->s6_addr[6] == 0)
      && (a->s6_addr[7] == 0)
      && (a->s6_addr[8] == 0)
      && (a->s6_addr[9] == 0)
      && (a->s6_addr[10] == 0)
      && (a->s6_addr[11] == 0)
      && (a->s6_addr[12] == 0)
      && (a->s6_addr[13] == 0)
      && (a->s6_addr[14] == 0)
      && (a->s6_addr[15] == 1));
}

inline int IN6_IS_ADDR_MULTICAST(const in6_addr_emulation* a)
{
  return (a->s6_addr[0] == 0xff);
}

inline int IN6_IS_ADDR_LINKLOCAL(const in6_addr_emulation* a)
{
  return ((a->s6_addr[0] == 0xfe) && ((a->s6_addr[1] & 0xc0) == 0x80));
}

inline int IN6_IS_ADDR_SITELOCAL(const in6_addr_emulation* a)
{
  return ((a->s6_addr[0] == 0xfe) && ((a->s6_addr[1] & 0xc0) == 0xc0));
}

inline int IN6_IS_ADDR_V4MAPPED(const in6_addr_emulation* a)
{
  return ((a->s6_addr[0] == 0)
      && (a->s6_addr[1] == 0)
      && (a->s6_addr[2] == 0)
      && (a->s6_addr[3] == 0)
      && (a->s6_addr[4] == 0)
      && (a->s6_addr[5] == 0)
      && (a->s6_addr[6] == 0)
      && (a->s6_addr[7] == 0)
      && (a->s6_addr[8] == 0)
      && (a->s6_addr[9] == 0)
      && (a->s6_addr[10] == 0xff)
      && (a->s6_addr[11] == 0xff));
}

inline int IN6_IS_ADDR_V4COMPAT(const in6_addr_emulation* a)
{
  return ((a->s6_addr[0] == 0)
      && (a->s6_addr[1] == 0)
      && (a->s6_addr[2] == 0)
      && (a->s6_addr[3] == 0)
      && (a->s6_addr[4] == 0)
      && (a->s6_addr[5] == 0)
      && (a->s6_addr[6] == 0)
      && (a->s6_addr[7] == 0)
      && (a->s6_addr[8] == 0)
      && (a->s6_addr[9] == 0)
      && (a->s6_addr[10] == 0xff)
      && (a->s6_addr[11] == 0xff)
      && !((a->s6_addr[12] == 0)
        && (a->s6_addr[13] == 0)
        && (a->s6_addr[14] == 0)
        && ((a->s6_addr[15] == 0) || (a->s6_addr[15] == 1))));
}

inline int IN6_IS_ADDR_MC_NODELOCAL(const in6_addr_emulation* a)
{
  return IN6_IS_ADDR_MULTICAST(a) && ((a->s6_addr[1] & 0xf) == 1);
}

inline int IN6_IS_ADDR_MC_LINKLOCAL(const in6_addr_emulation* a)
{
  return IN6_IS_ADDR_MULTICAST(a) && ((a->s6_addr[1] & 0xf) == 2);
}

inline int IN6_IS_ADDR_MC_SITELOCAL(const in6_addr_emulation* a)
{
  return IN6_IS_ADDR_MULTICAST(a) && ((a->s6_addr[1] & 0xf) == 5);
}

inline int IN6_IS_ADDR_MC_ORGLOCAL(const in6_addr_emulation* a)
{
  return IN6_IS_ADDR_MULTICAST(a) && ((a->s6_addr[1] & 0xf) == 8);
}

inline int IN6_IS_ADDR_MC_GLOBAL(const in6_addr_emulation* a)
{
  return IN6_IS_ADDR_MULTICAST(a) && ((a->s6_addr[1] & 0xf) == 0xe);
}

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // defined(BOOST_ASIO_HAS_OLD_WIN_SDK)

// Even newer Platform SDKs that support IPv6 may not define IPV6_V6ONLY.
#if !defined(IPV6_V6ONLY)
# define IPV6_V6ONLY 27
#endif

// Some SDKs (e.g. Windows CE) don't define IPPROTO_ICMPV6.
#if !defined(IPPROTO_ICMPV6)
# define IPPROTO_ICMPV6 58
#endif

#endif // defined(BOOST_WINDOWS) || defined(__CYGWIN__)

#endif // BOOST_ASIO_DETAIL_OLD_WIN_SDK_COMPAT_HPP
