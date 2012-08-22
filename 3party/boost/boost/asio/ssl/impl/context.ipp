//
// ssl/impl/context.ipp
// ~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2005 Voipster / Indrek dot Juhani at voipster dot com
// Copyright (c) 2005-2012 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_SSL_IMPL_CONTEXT_IPP
#define BOOST_ASIO_SSL_IMPL_CONTEXT_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if !defined(BOOST_ASIO_ENABLE_OLD_SSL)
# include <cstring>
# include <boost/asio/detail/throw_error.hpp>
# include <boost/asio/error.hpp>
# include <boost/asio/ssl/context.hpp>
# include <boost/asio/ssl/error.hpp>
#endif // !defined(BOOST_ASIO_ENABLE_OLD_SSL)

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace ssl {

#if !defined(BOOST_ASIO_ENABLE_OLD_SSL)

context::context(context::method m)
  : handle_(0)
{
  switch (m)
  {
#if defined(OPENSSL_NO_SSL2)
  case context::sslv2:
  case context::sslv2_client:
  case context::sslv2_server:
    boost::asio::detail::throw_error(
        boost::asio::error::invalid_argument, "context");
    break;
#else // defined(OPENSSL_NO_SSL2)
  case context::sslv2:
    handle_ = ::SSL_CTX_new(::SSLv2_method());
    break;
  case context::sslv2_client:
    handle_ = ::SSL_CTX_new(::SSLv2_client_method());
    break;
  case context::sslv2_server:
    handle_ = ::SSL_CTX_new(::SSLv2_server_method());
    break;
#endif // defined(OPENSSL_NO_SSL2)
  case context::sslv3:
    handle_ = ::SSL_CTX_new(::SSLv3_method());
    break;
  case context::sslv3_client:
    handle_ = ::SSL_CTX_new(::SSLv3_client_method());
    break;
  case context::sslv3_server:
    handle_ = ::SSL_CTX_new(::SSLv3_server_method());
    break;
  case context::tlsv1:
    handle_ = ::SSL_CTX_new(::TLSv1_method());
    break;
  case context::tlsv1_client:
    handle_ = ::SSL_CTX_new(::TLSv1_client_method());
    break;
  case context::tlsv1_server:
    handle_ = ::SSL_CTX_new(::TLSv1_server_method());
    break;
  case context::sslv23:
    handle_ = ::SSL_CTX_new(::SSLv23_method());
    break;
  case context::sslv23_client:
    handle_ = ::SSL_CTX_new(::SSLv23_client_method());
    break;
  case context::sslv23_server:
    handle_ = ::SSL_CTX_new(::SSLv23_server_method());
    break;
  default:
    handle_ = ::SSL_CTX_new(0);
    break;
  }

  if (handle_ == 0)
  {
    boost::system::error_code ec(::ERR_get_error(),
        boost::asio::error::get_ssl_category());
    boost::asio::detail::throw_error(ec, "context");
  }
}

context::context(boost::asio::io_service&, context::method m)
  : handle_(0)
{
  context tmp(m);
  handle_ = tmp.handle_;
  tmp.handle_ = 0;
}

#if defined(BOOST_ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)
context::context(context&& other)
{
  handle_ = other.handle_;
  other.handle_ = 0;
}

context& context::operator=(context&& other)
{
  context tmp(BOOST_ASIO_MOVE_CAST(context)(*this));
  handle_ = other.handle_;
  other.handle_ = 0;
  return *this;
}
#endif // defined(BOOST_ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)

context::~context()
{
  if (handle_)
  {
    if (handle_->default_passwd_callback_userdata)
    {
      detail::password_callback_base* callback =
        static_cast<detail::password_callback_base*>(
            handle_->default_passwd_callback_userdata);
      delete callback;
      handle_->default_passwd_callback_userdata = 0;
    }

    if (SSL_CTX_get_app_data(handle_))
    {
      detail::verify_callback_base* callback =
        static_cast<detail::verify_callback_base*>(
            SSL_CTX_get_app_data(handle_));
      delete callback;
      SSL_CTX_set_app_data(handle_, 0);
    }

    ::SSL_CTX_free(handle_);
  }
}

context::native_handle_type context::native_handle()
{
  return handle_;
}

context::impl_type context::impl()
{
  return handle_;
}

void context::set_options(context::options o)
{
  boost::system::error_code ec;
  set_options(o, ec);
  boost::asio::detail::throw_error(ec, "set_options");
}

boost::system::error_code context::set_options(
    context::options o, boost::system::error_code& ec)
{
  ::SSL_CTX_set_options(handle_, o);

  ec = boost::system::error_code();
  return ec;
}

void context::set_verify_mode(verify_mode v)
{
  boost::system::error_code ec;
  set_verify_mode(v, ec);
  boost::asio::detail::throw_error(ec, "set_verify_mode");
}

boost::system::error_code context::set_verify_mode(
    verify_mode v, boost::system::error_code& ec)
{
  ::SSL_CTX_set_verify(handle_, v, ::SSL_CTX_get_verify_callback(handle_));

  ec = boost::system::error_code();
  return ec;
}

void context::load_verify_file(const std::string& filename)
{
  boost::system::error_code ec;
  load_verify_file(filename, ec);
  boost::asio::detail::throw_error(ec, "load_verify_file");
}

boost::system::error_code context::load_verify_file(
    const std::string& filename, boost::system::error_code& ec)
{
  if (::SSL_CTX_load_verify_locations(handle_, filename.c_str(), 0) != 1)
  {
    ec = boost::system::error_code(::ERR_get_error(),
        boost::asio::error::get_ssl_category());
    return ec;
  }

  ec = boost::system::error_code();
  return ec;
}

void context::set_default_verify_paths()
{
  boost::system::error_code ec;
  set_default_verify_paths(ec);
  boost::asio::detail::throw_error(ec, "set_default_verify_paths");
}

boost::system::error_code context::set_default_verify_paths(
    boost::system::error_code& ec)
{
  if (::SSL_CTX_set_default_verify_paths(handle_) != 1)
  {
    ec = boost::system::error_code(::ERR_get_error(),
        boost::asio::error::get_ssl_category());
    return ec;
  }

  ec = boost::system::error_code();
  return ec;
}

void context::add_verify_path(const std::string& path)
{
  boost::system::error_code ec;
  add_verify_path(path, ec);
  boost::asio::detail::throw_error(ec, "add_verify_path");
}

boost::system::error_code context::add_verify_path(
    const std::string& path, boost::system::error_code& ec)
{
  if (::SSL_CTX_load_verify_locations(handle_, 0, path.c_str()) != 1)
  {
    ec = boost::system::error_code(::ERR_get_error(),
        boost::asio::error::get_ssl_category());
    return ec;
  }

  ec = boost::system::error_code();
  return ec;
}

void context::use_certificate_file(
    const std::string& filename, file_format format)
{
  boost::system::error_code ec;
  use_certificate_file(filename, format, ec);
  boost::asio::detail::throw_error(ec, "use_certificate_file");
}

boost::system::error_code context::use_certificate_file(
    const std::string& filename, file_format format,
    boost::system::error_code& ec)
{
  int file_type;
  switch (format)
  {
  case context_base::asn1:
    file_type = SSL_FILETYPE_ASN1;
    break;
  case context_base::pem:
    file_type = SSL_FILETYPE_PEM;
    break;
  default:
    {
      ec = boost::asio::error::invalid_argument;
      return ec;
    }
  }

  if (::SSL_CTX_use_certificate_file(handle_, filename.c_str(), file_type) != 1)
  {
    ec = boost::system::error_code(::ERR_get_error(),
        boost::asio::error::get_ssl_category());
    return ec;
  }

  ec = boost::system::error_code();
  return ec;
}

void context::use_certificate_chain_file(const std::string& filename)
{
  boost::system::error_code ec;
  use_certificate_chain_file(filename, ec);
  boost::asio::detail::throw_error(ec, "use_certificate_chain_file");
}

boost::system::error_code context::use_certificate_chain_file(
    const std::string& filename, boost::system::error_code& ec)
{
  if (::SSL_CTX_use_certificate_chain_file(handle_, filename.c_str()) != 1)
  {
    ec = boost::system::error_code(::ERR_get_error(),
        boost::asio::error::get_ssl_category());
    return ec;
  }

  ec = boost::system::error_code();
  return ec;
}

void context::use_private_key_file(
    const std::string& filename, context::file_format format)
{
  boost::system::error_code ec;
  use_private_key_file(filename, format, ec);
  boost::asio::detail::throw_error(ec, "use_private_key_file");
}

boost::system::error_code context::use_private_key_file(
    const std::string& filename, context::file_format format,
    boost::system::error_code& ec)
{
  int file_type;
  switch (format)
  {
  case context_base::asn1:
    file_type = SSL_FILETYPE_ASN1;
    break;
  case context_base::pem:
    file_type = SSL_FILETYPE_PEM;
    break;
  default:
    {
      ec = boost::asio::error::invalid_argument;
      return ec;
    }
  }

  if (::SSL_CTX_use_PrivateKey_file(handle_, filename.c_str(), file_type) != 1)
  {
    ec = boost::system::error_code(::ERR_get_error(),
        boost::asio::error::get_ssl_category());
    return ec;
  }

  ec = boost::system::error_code();
  return ec;
}

void context::use_rsa_private_key_file(
    const std::string& filename, context::file_format format)
{
  boost::system::error_code ec;
  use_rsa_private_key_file(filename, format, ec);
  boost::asio::detail::throw_error(ec, "use_rsa_private_key_file");
}

boost::system::error_code context::use_rsa_private_key_file(
    const std::string& filename, context::file_format format,
    boost::system::error_code& ec)
{
  int file_type;
  switch (format)
  {
  case context_base::asn1:
    file_type = SSL_FILETYPE_ASN1;
    break;
  case context_base::pem:
    file_type = SSL_FILETYPE_PEM;
    break;
  default:
    {
      ec = boost::asio::error::invalid_argument;
      return ec;
    }
  }

  if (::SSL_CTX_use_RSAPrivateKey_file(
        handle_, filename.c_str(), file_type) != 1)
  {
    ec = boost::system::error_code(::ERR_get_error(),
        boost::asio::error::get_ssl_category());
    return ec;
  }

  ec = boost::system::error_code();
  return ec;
}

void context::use_tmp_dh_file(const std::string& filename)
{
  boost::system::error_code ec;
  use_tmp_dh_file(filename, ec);
  boost::asio::detail::throw_error(ec, "use_tmp_dh_file");
}

boost::system::error_code context::use_tmp_dh_file(
    const std::string& filename, boost::system::error_code& ec)
{
  ::BIO* bio = ::BIO_new_file(filename.c_str(), "r");
  if (!bio)
  {
    ec = boost::asio::error::invalid_argument;
    return ec;
  }

  ::DH* dh = ::PEM_read_bio_DHparams(bio, 0, 0, 0);
  if (!dh)
  {
    ::BIO_free(bio);
    ec = boost::asio::error::invalid_argument;
    return ec;
  }

  ::BIO_free(bio);
  int result = ::SSL_CTX_set_tmp_dh(handle_, dh);
  ::DH_free(dh);
  if (result != 1)
  {
    ec = boost::system::error_code(::ERR_get_error(),
        boost::asio::error::get_ssl_category());
    return ec;
  }

  ec = boost::system::error_code();
  return ec;
}

boost::system::error_code context::do_set_verify_callback(
    detail::verify_callback_base* callback, boost::system::error_code& ec)
{
  if (SSL_CTX_get_app_data(handle_))
  {
    delete static_cast<detail::verify_callback_base*>(
        SSL_CTX_get_app_data(handle_));
  }

  SSL_CTX_set_app_data(handle_, callback);

  ::SSL_CTX_set_verify(handle_,
      ::SSL_CTX_get_verify_mode(handle_),
      &context::verify_callback_function);

  ec = boost::system::error_code();
  return ec;
}

int context::verify_callback_function(int preverified, X509_STORE_CTX* ctx)
{
  if (ctx)
  {
    if (SSL* ssl = static_cast<SSL*>(
          ::X509_STORE_CTX_get_ex_data(
            ctx, ::SSL_get_ex_data_X509_STORE_CTX_idx())))
    {
      if (SSL_CTX* handle = ::SSL_get_SSL_CTX(ssl))
      {
        if (SSL_CTX_get_app_data(handle))
        {
          detail::verify_callback_base* callback =
            static_cast<detail::verify_callback_base*>(
                SSL_CTX_get_app_data(handle));

          verify_context verify_ctx(ctx);
          return callback->call(preverified != 0, verify_ctx) ? 1 : 0;
        }
      }
    }
  }

  return 0;
}

boost::system::error_code context::do_set_password_callback(
    detail::password_callback_base* callback, boost::system::error_code& ec)
{
  if (handle_->default_passwd_callback_userdata)
    delete static_cast<detail::password_callback_base*>(
        handle_->default_passwd_callback_userdata);

  handle_->default_passwd_callback_userdata = callback;

  SSL_CTX_set_default_passwd_cb(handle_, &context::password_callback_function);

  ec = boost::system::error_code();
  return ec;
}

int context::password_callback_function(
    char* buf, int size, int purpose, void* data)
{
  using namespace std; // For strncat and strlen.

  if (data)
  {
    detail::password_callback_base* callback =
      static_cast<detail::password_callback_base*>(data);

    std::string passwd = callback->call(static_cast<std::size_t>(size),
        purpose ? context_base::for_writing : context_base::for_reading);

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400) && !defined(UNDER_CE)
    strcpy_s(buf, size, passwd.c_str());
#else
    *buf = '\0';
    strncat(buf, passwd.c_str(), size);
#endif

    return strlen(buf);
  }

  return 0;
}

#endif // !defined(BOOST_ASIO_ENABLE_OLD_SSL)

} // namespace ssl
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_SSL_IMPL_CONTEXT_IPP
