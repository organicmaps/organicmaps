//
// ip/impl/address_v6.ipp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2010 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_IP_IMPL_ADDRESS_V6_IPP
#define BOOST_ASIO_IP_IMPL_ADDRESS_V6_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <cstring>
#include <stdexcept>
#include <typeinfo>
#include <boost/throw_exception.hpp>
#include <boost/asio/detail/socket_ops.hpp>
#include <boost/asio/detail/throw_error.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/ip/address_v6.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace ip {

address_v6::address_v6()
  : scope_id_(0)
{
  boost::asio::detail::in6_addr_type tmp_addr = IN6ADDR_ANY_INIT;
  addr_ = tmp_addr;
}

address_v6::address_v6(const address_v6::bytes_type& bytes,
    unsigned long scope_id)
  : scope_id_(scope_id)
{
#if UCHAR_MAX > 0xFF
  for (std::size_t i = 0; i < bytes.size(); ++i)
  {
    if (bytes[i] > 0xFF)
    {
      std::out_of_range ex("address_v6 from bytes_type");
      boost::throw_exception(ex);
    }
  }
#endif // UCHAR_MAX > 0xFF

  using namespace std; // For memcpy.
  memcpy(addr_.s6_addr, bytes.elems, 16);
}

address_v6::address_v6(const address_v6& other)
  : addr_(other.addr_),
    scope_id_(other.scope_id_)
{
}

address_v6& address_v6::operator=(const address_v6& other)
{
  addr_ = other.addr_;
  scope_id_ = other.scope_id_;
  return *this;
}

address_v6::bytes_type address_v6::to_bytes() const
{
  using namespace std; // For memcpy.
  bytes_type bytes;
  memcpy(bytes.elems, addr_.s6_addr, 16);
  return bytes;
}

std::string address_v6::to_string() const
{
  boost::system::error_code ec;
  std::string addr = to_string(ec);
  boost::asio::detail::throw_error(ec);
  return addr;
}

std::string address_v6::to_string(boost::system::error_code& ec) const
{
  char addr_str[boost::asio::detail::max_addr_v6_str_len];
  const char* addr =
    boost::asio::detail::socket_ops::inet_ntop(AF_INET6, &addr_, addr_str,
        boost::asio::detail::max_addr_v6_str_len, scope_id_, ec);
  if (addr == 0)
    return std::string();
  return addr;
}

address_v6 address_v6::from_string(const char* str)
{
  boost::system::error_code ec;
  address_v6 addr = from_string(str, ec);
  boost::asio::detail::throw_error(ec);
  return addr;
}

address_v6 address_v6::from_string(
    const char* str, boost::system::error_code& ec)
{
  address_v6 tmp;
  if (boost::asio::detail::socket_ops::inet_pton(
        AF_INET6, str, &tmp.addr_, &tmp.scope_id_, ec) <= 0)
    return address_v6();
  return tmp;
}

address_v6 address_v6::from_string(const std::string& str)
{
  return from_string(str.c_str());
}

address_v6 address_v6::from_string(
    const std::string& str, boost::system::error_code& ec)
{
  return from_string(str.c_str(), ec);
}

address_v4 address_v6::to_v4() const
{
  if (!is_v4_mapped() && !is_v4_compatible())
  {
    std::bad_cast ex;
    boost::throw_exception(ex);
  }

  address_v4::bytes_type v4_bytes = { { addr_.s6_addr[12],
    addr_.s6_addr[13], addr_.s6_addr[14], addr_.s6_addr[15] } };
  return address_v4(v4_bytes);
}

bool address_v6::is_loopback() const
{
#if defined(__BORLANDC__)
  return ((addr_.s6_addr[0] == 0) && (addr_.s6_addr[1] == 0)
      && (addr_.s6_addr[2] == 0) && (addr_.s6_addr[3] == 0)
      && (addr_.s6_addr[4] == 0) && (addr_.s6_addr[5] == 0)
      && (addr_.s6_addr[6] == 0) && (addr_.s6_addr[7] == 0)
      && (addr_.s6_addr[8] == 0) && (addr_.s6_addr[9] == 0)
      && (addr_.s6_addr[10] == 0) && (addr_.s6_addr[11] == 0)
      && (addr_.s6_addr[12] == 0) && (addr_.s6_addr[13] == 0)
      && (addr_.s6_addr[14] == 0) && (addr_.s6_addr[15] == 1));
#else
  using namespace boost::asio::detail;
  return IN6_IS_ADDR_LOOPBACK(&addr_) != 0;
#endif
}

bool address_v6::is_unspecified() const
{
#if defined(__BORLANDC__)
  return ((addr_.s6_addr[0] == 0) && (addr_.s6_addr[1] == 0)
      && (addr_.s6_addr[2] == 0) && (addr_.s6_addr[3] == 0)
      && (addr_.s6_addr[4] == 0) && (addr_.s6_addr[5] == 0)
      && (addr_.s6_addr[6] == 0) && (addr_.s6_addr[7] == 0)
      && (addr_.s6_addr[8] == 0) && (addr_.s6_addr[9] == 0)
      && (addr_.s6_addr[10] == 0) && (addr_.s6_addr[11] == 0)
      && (addr_.s6_addr[12] == 0) && (addr_.s6_addr[13] == 0)
      && (addr_.s6_addr[14] == 0) && (addr_.s6_addr[15] == 0));
#else
  using namespace boost::asio::detail;
  return IN6_IS_ADDR_UNSPECIFIED(&addr_) != 0;
#endif
}

bool address_v6::is_link_local() const
{
  using namespace boost::asio::detail;
  return IN6_IS_ADDR_LINKLOCAL(&addr_) != 0;
}

bool address_v6::is_site_local() const
{
  using namespace boost::asio::detail;
  return IN6_IS_ADDR_SITELOCAL(&addr_) != 0;
}

bool address_v6::is_v4_mapped() const
{
  using namespace boost::asio::detail;
  return IN6_IS_ADDR_V4MAPPED(&addr_) != 0;
}

bool address_v6::is_v4_compatible() const
{
  using namespace boost::asio::detail;
  return IN6_IS_ADDR_V4COMPAT(&addr_) != 0;
}

bool address_v6::is_multicast() const
{
  using namespace boost::asio::detail;
  return IN6_IS_ADDR_MULTICAST(&addr_) != 0;
}

bool address_v6::is_multicast_global() const
{
  using namespace boost::asio::detail;
  return IN6_IS_ADDR_MC_GLOBAL(&addr_) != 0;
}

bool address_v6::is_multicast_link_local() const
{
  using namespace boost::asio::detail;
  return IN6_IS_ADDR_MC_LINKLOCAL(&addr_) != 0;
}

bool address_v6::is_multicast_node_local() const
{
  using namespace boost::asio::detail;
  return IN6_IS_ADDR_MC_NODELOCAL(&addr_) != 0;
}

bool address_v6::is_multicast_org_local() const
{
  using namespace boost::asio::detail;
  return IN6_IS_ADDR_MC_ORGLOCAL(&addr_) != 0;
}

bool address_v6::is_multicast_site_local() const
{
  using namespace boost::asio::detail;
  return IN6_IS_ADDR_MC_SITELOCAL(&addr_) != 0;
}

bool operator==(const address_v6& a1, const address_v6& a2)
{
  using namespace std; // For memcmp.
  return memcmp(&a1.addr_, &a2.addr_,
      sizeof(boost::asio::detail::in6_addr_type)) == 0
    && a1.scope_id_ == a2.scope_id_;
}

bool operator<(const address_v6& a1, const address_v6& a2)
{
  using namespace std; // For memcmp.
  int memcmp_result = memcmp(&a1.addr_, &a2.addr_,
      sizeof(boost::asio::detail::in6_addr_type));
  if (memcmp_result < 0)
    return true;
  if (memcmp_result > 0)
    return false;
  return a1.scope_id_ < a2.scope_id_;
}

address_v6 address_v6::loopback()
{
  address_v6 tmp;
  boost::asio::detail::in6_addr_type tmp_addr = IN6ADDR_LOOPBACK_INIT;
  tmp.addr_ = tmp_addr;
  return tmp;
}

address_v6 address_v6::v4_mapped(const address_v4& addr)
{
  address_v4::bytes_type v4_bytes = addr.to_bytes();
  bytes_type v6_bytes = { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF,
    v4_bytes[0], v4_bytes[1], v4_bytes[2], v4_bytes[3] } };
  return address_v6(v6_bytes);
}

address_v6 address_v6::v4_compatible(const address_v4& addr)
{
  address_v4::bytes_type v4_bytes = addr.to_bytes();
  bytes_type v6_bytes = { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    v4_bytes[0], v4_bytes[1], v4_bytes[2], v4_bytes[3] } };
  return address_v6(v6_bytes);
}

} // namespace ip
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_IP_IMPL_ADDRESS_V6_IPP
