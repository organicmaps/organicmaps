// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_DETAIL_WINDOWS_ASYNC_PIPE_HPP_
#define BOOST_PROCESS_DETAIL_WINDOWS_ASYNC_PIPE_HPP_

#include <boost/detail/winapi/basic_types.hpp>
#include <boost/detail/winapi/pipes.hpp>
#include <boost/detail/winapi/handles.hpp>
#include <boost/detail/winapi/file_management.hpp>
#include <boost/detail/winapi/get_last_error.hpp>
#include <boost/detail/winapi/access_rights.hpp>
#include <boost/detail/winapi/process.hpp>
#include <boost/process/detail/windows/basic_pipe.hpp>
#include <boost/asio/windows/stream_handle.hpp>
#include <system_error>
#include <string>

namespace boost { namespace process { namespace detail { namespace windows {

inline std::string make_pipe_name()
{
    std::string name = "\\\\.\\pipe\\boost_process_auto_pipe_";

    auto pid = ::boost::detail::winapi::GetCurrentProcessId();

    static unsigned long long cnt = 0;
    name += std::to_string(pid);
    name += "_";
    name += std::to_string(cnt++);

    return name;
}

class async_pipe
{
    ::boost::asio::windows::stream_handle _source;
    ::boost::asio::windows::stream_handle _sink  ;
public:
    typedef ::boost::detail::winapi::HANDLE_ native_handle_type;
    typedef ::boost::asio::windows::stream_handle   handle_type;

    inline async_pipe(boost::asio::io_service & ios,
                      const std::string & name = make_pipe_name())
                    : async_pipe(ios, ios, name) {}

    inline async_pipe(boost::asio::io_service & ios_source,
                      boost::asio::io_service & ios_sink,
                      const std::string & name = make_pipe_name());

    inline async_pipe(const async_pipe& rhs);
    async_pipe(async_pipe&& rhs)  : _source(std::move(rhs._source)), _sink(std::move(rhs._sink))
    {
        rhs._source.assign (::boost::detail::winapi::INVALID_HANDLE_VALUE_);
        rhs._sink  .assign (::boost::detail::winapi::INVALID_HANDLE_VALUE_);
    }
    template<class CharT, class Traits = std::char_traits<CharT>>
    explicit async_pipe(::boost::asio::io_service & ios_source,
                        ::boost::asio::io_service & ios_sink,
                         const basic_pipe<CharT, Traits> & p)
            : _source(ios_source, p.native_source()), _sink(ios_sink, p.native_sink())
    {
    }

    template<class CharT, class Traits = std::char_traits<CharT>>
    explicit async_pipe(boost::asio::io_service & ios, const basic_pipe<CharT, Traits> & p)
            : async_pipe(ios, ios, p)
    {
    }

    template<class CharT, class Traits = std::char_traits<CharT>>
    inline async_pipe& operator=(const basic_pipe<CharT, Traits>& p);
    inline async_pipe& operator=(const async_pipe& rhs);

    inline async_pipe& operator=(async_pipe&& rhs);

    ~async_pipe()
    {
        if (_sink .native()  != ::boost::detail::winapi::INVALID_HANDLE_VALUE_)
            ::boost::detail::winapi::CloseHandle(_sink.native());
        if (_source.native() != ::boost::detail::winapi::INVALID_HANDLE_VALUE_)
            ::boost::detail::winapi::CloseHandle(_source.native());
    }

    template<class CharT, class Traits = std::char_traits<CharT>>
    inline explicit operator basic_pipe<CharT, Traits>() const;

    void cancel()
    {
        if (_sink.is_open())
            _sink.cancel();
        if (_source.is_open())
            _source.cancel();
    }

    void close()
    {
        if (_sink.is_open())
        {
            _sink.close();
            _sink = handle_type(_sink.get_io_service());
        }
        if (_source.is_open())
        {
            _source.close();
            _source = handle_type(_source.get_io_service());
        }
    }
    void close(boost::system::error_code & ec)
    {
        if (_sink.is_open())
        {
            _sink.close(ec);
            _sink = handle_type(_sink.get_io_service());
        }
        if (_source.is_open())
        {
            _source.close(ec);
            _source = handle_type(_source.get_io_service());
        }
    }

    bool is_open() const
    {
        return  _sink.is_open() || _source.is_open();
    }
    void async_close()
    {
        if (_sink.is_open())
            _sink.get_io_service().  post([this]{_sink.close();});
        if (_source.is_open())
            _source.get_io_service().post([this]{_source.close();});
    }

    template<typename MutableBufferSequence>
    std::size_t read_some(const MutableBufferSequence & buffers)
    {
        return _source.read_some(buffers);
    }
    template<typename MutableBufferSequence>
    std::size_t write_some(const MutableBufferSequence & buffers)
    {
        return _sink.write_some(buffers);
    }

    native_handle_type native_source() const {return const_cast<boost::asio::windows::stream_handle&>(_source).native();}
    native_handle_type native_sink  () const {return const_cast<boost::asio::windows::stream_handle&>(_sink  ).native();}

    template<typename MutableBufferSequence,
             typename ReadHandler>
    BOOST_ASIO_INITFN_RESULT_TYPE(
          ReadHandler, void(boost::system::error_code, std::size_t))
      async_read_some(
        const MutableBufferSequence & buffers,
              ReadHandler &&handler)
    {
        _source.async_read_some(buffers, std::forward<ReadHandler>(handler));
    }

    template<typename ConstBufferSequence,
             typename WriteHandler>
    BOOST_ASIO_INITFN_RESULT_TYPE(
              WriteHandler, void(boost::system::error_code, std::size_t))
      async_write_some(
        const ConstBufferSequence & buffers,
        WriteHandler && handler)
    {
        _sink.async_write_some(buffers,  std::forward<WriteHandler>(handler));
    }

    const handle_type & sink  () const & {return _sink;}
    const handle_type & source() const & {return _source;}

    handle_type && source() && { return std::move(_source); }
    handle_type && sink()   && { return std::move(_sink); }

    handle_type source(::boost::asio::io_service& ios) &&
    {
        ::boost::asio::windows::stream_handle stolen(ios, _source.native_handle());
        _source.assign(::boost::detail::winapi::INVALID_HANDLE_VALUE_);
        return stolen;
    }
    handle_type sink  (::boost::asio::io_service& ios) &&
    {
        ::boost::asio::windows::stream_handle stolen(ios, _sink.native_handle());
        _sink.assign(::boost::detail::winapi::INVALID_HANDLE_VALUE_);
        return stolen;
    }

    handle_type source(::boost::asio::io_service& ios) const &
    {
        auto proc = ::boost::detail::winapi::GetCurrentProcess();

        ::boost::detail::winapi::HANDLE_ source;
        auto source_in = const_cast<handle_type&>(_source).native();
        if (source_in == ::boost::detail::winapi::INVALID_HANDLE_VALUE_)
            source = ::boost::detail::winapi::INVALID_HANDLE_VALUE_;
        else if (!::boost::detail::winapi::DuplicateHandle(
                proc, source_in, proc, &source, 0,
                static_cast<::boost::detail::winapi::BOOL_>(true),
                 ::boost::detail::winapi::DUPLICATE_SAME_ACCESS_))
            throw_last_error("Duplicate Pipe Failed");

        return ::boost::asio::windows::stream_handle(ios, source);
    }
    handle_type sink  (::boost::asio::io_service& ios) const &
    {
        auto proc = ::boost::detail::winapi::GetCurrentProcess();

        ::boost::detail::winapi::HANDLE_ sink;
        auto sink_in = const_cast<handle_type&>(_sink).native();
        if (sink_in == ::boost::detail::winapi::INVALID_HANDLE_VALUE_)
            sink = ::boost::detail::winapi::INVALID_HANDLE_VALUE_;
        else if (!::boost::detail::winapi::DuplicateHandle(
                proc, sink_in, proc, &sink, 0,
                static_cast<::boost::detail::winapi::BOOL_>(true),
                 ::boost::detail::winapi::DUPLICATE_SAME_ACCESS_))
            throw_last_error("Duplicate Pipe Failed");

        return ::boost::asio::windows::stream_handle(ios, sink);
    }
};



async_pipe::async_pipe(const async_pipe& p)  :
    _source(const_cast<handle_type&>(p._source).get_io_service()),
    _sink  (const_cast<handle_type&>(p._sink).get_io_service())
{
    auto proc = ::boost::detail::winapi::GetCurrentProcess();

    ::boost::detail::winapi::HANDLE_ source;
    ::boost::detail::winapi::HANDLE_ sink;

    //cannot get the handle from a const object.
    auto source_in = const_cast<handle_type&>(p._source).native();
    auto sink_in   = const_cast<handle_type&>(p._sink).native();

    if (source_in == ::boost::detail::winapi::INVALID_HANDLE_VALUE_)
        source = ::boost::detail::winapi::INVALID_HANDLE_VALUE_;
    else if (!::boost::detail::winapi::DuplicateHandle(
            proc, source_in, proc, &source, 0,
            static_cast<::boost::detail::winapi::BOOL_>(true),
             ::boost::detail::winapi::DUPLICATE_SAME_ACCESS_))
        throw_last_error("Duplicate Pipe Failed");

    if (sink_in   == ::boost::detail::winapi::INVALID_HANDLE_VALUE_)
        sink = ::boost::detail::winapi::INVALID_HANDLE_VALUE_;
    else if (!::boost::detail::winapi::DuplicateHandle(
            proc, sink_in, proc, &sink, 0,
            static_cast<::boost::detail::winapi::BOOL_>(true),
             ::boost::detail::winapi::DUPLICATE_SAME_ACCESS_))
        throw_last_error("Duplicate Pipe Failed");

    _source.assign(source);
    _sink.  assign(sink);
}


async_pipe::async_pipe(boost::asio::io_service & ios_source,
                       boost::asio::io_service & ios_sink,
                       const std::string & name) : _source(ios_source), _sink(ios_sink)
{
    static constexpr int FILE_FLAG_OVERLAPPED_  = 0x40000000; //temporary

    ::boost::detail::winapi::HANDLE_ source = ::boost::detail::winapi::create_named_pipe(
            name.c_str(),
            ::boost::detail::winapi::PIPE_ACCESS_INBOUND_
            | FILE_FLAG_OVERLAPPED_, //write flag
            0, 1, 8192, 8192, 0, nullptr);


    if (source == boost::detail::winapi::INVALID_HANDLE_VALUE_)
        ::boost::process::detail::throw_last_error("create_named_pipe(" + name + ") failed");

    _source.assign(source);

    ::boost::detail::winapi::HANDLE_ sink = boost::detail::winapi::create_file(
            name.c_str(),
            ::boost::detail::winapi::GENERIC_WRITE_, 0, nullptr,
            ::boost::detail::winapi::OPEN_EXISTING_,
            FILE_FLAG_OVERLAPPED_, //to allow read
            nullptr);

    if (sink == ::boost::detail::winapi::INVALID_HANDLE_VALUE_)
        ::boost::process::detail::throw_last_error("create_file() failed");

    _sink.assign(sink);
}

async_pipe& async_pipe::operator=(const async_pipe & p)
{
    auto proc = ::boost::detail::winapi::GetCurrentProcess();

    ::boost::detail::winapi::HANDLE_ source;
    ::boost::detail::winapi::HANDLE_ sink;

    //cannot get the handle from a const object.
    auto &source_in = const_cast<::boost::asio::windows::stream_handle &>(p._source);
    auto &sink_in   = const_cast<::boost::asio::windows::stream_handle &>(p._sink);

    if (source_in.native() == ::boost::detail::winapi::INVALID_HANDLE_VALUE_)
        source = ::boost::detail::winapi::INVALID_HANDLE_VALUE_;
    else if (!::boost::detail::winapi::DuplicateHandle(
            proc, source_in.native(), proc, &source, 0,
            static_cast<::boost::detail::winapi::BOOL_>(true),
             ::boost::detail::winapi::DUPLICATE_SAME_ACCESS_))
        throw_last_error("Duplicate Pipe Failed");

    if (sink_in.native()   == ::boost::detail::winapi::INVALID_HANDLE_VALUE_)
        sink = ::boost::detail::winapi::INVALID_HANDLE_VALUE_;
    else if (!::boost::detail::winapi::DuplicateHandle(
            proc, sink_in.native(), proc, &sink, 0,
            static_cast<::boost::detail::winapi::BOOL_>(true),
             ::boost::detail::winapi::DUPLICATE_SAME_ACCESS_))
        throw_last_error("Duplicate Pipe Failed");

    //so we also assign the io_service
    _source = ::boost::asio::windows::stream_handle(source_in.get_io_service(), source);
    _sink = ::boost::asio::windows::stream_handle(source_in.get_io_service(), sink);

    return *this;
}

async_pipe& async_pipe::operator=(async_pipe && rhs)
{
    if (_source.native_handle() != ::boost::detail::winapi::INVALID_HANDLE_VALUE_)
        ::boost::detail::winapi::CloseHandle(_source.native());

    if (_sink.native_handle()   != ::boost::detail::winapi::INVALID_HANDLE_VALUE_)
        ::boost::detail::winapi::CloseHandle(_sink.native());

    _source.assign(rhs._source.native_handle());
    _sink  .assign(rhs._sink  .native_handle());
    rhs._source.assign(::boost::detail::winapi::INVALID_HANDLE_VALUE_);
    rhs._sink  .assign(::boost::detail::winapi::INVALID_HANDLE_VALUE_);
    return *this;
}

template<class CharT, class Traits>
async_pipe::operator basic_pipe<CharT, Traits>() const
{
    auto proc = ::boost::detail::winapi::GetCurrentProcess();

    ::boost::detail::winapi::HANDLE_ source;
    ::boost::detail::winapi::HANDLE_ sink;

    //cannot get the handle from a const object.
    auto source_in = const_cast<::boost::asio::windows::stream_handle &>(_source).native();
    auto sink_in   = const_cast<::boost::asio::windows::stream_handle &>(_sink).native();

    if (source_in == ::boost::detail::winapi::INVALID_HANDLE_VALUE_)
        source = ::boost::detail::winapi::INVALID_HANDLE_VALUE_;
    else if (!::boost::detail::winapi::DuplicateHandle(
            proc, source_in, proc, &source, 0,
            static_cast<::boost::detail::winapi::BOOL_>(true),
             ::boost::detail::winapi::DUPLICATE_SAME_ACCESS_))
        throw_last_error("Duplicate Pipe Failed");

    if (sink_in == ::boost::detail::winapi::INVALID_HANDLE_VALUE_)
        sink = ::boost::detail::winapi::INVALID_HANDLE_VALUE_;
    else if (!::boost::detail::winapi::DuplicateHandle(
            proc, sink_in, proc, &sink, 0,
            static_cast<::boost::detail::winapi::BOOL_>(true),
             ::boost::detail::winapi::DUPLICATE_SAME_ACCESS_))
        throw_last_error("Duplicate Pipe Failed");

    return basic_pipe<CharT, Traits>{source, sink};
}

inline bool operator==(const async_pipe & lhs, const async_pipe & rhs)
{
    return compare_handles(lhs.native_source(), rhs.native_source()) &&
           compare_handles(lhs.native_sink(),   rhs.native_sink());
}

inline bool operator!=(const async_pipe & lhs, const async_pipe & rhs)
{
    return !compare_handles(lhs.native_source(), rhs.native_source()) ||
           !compare_handles(lhs.native_sink(),   rhs.native_sink());
}

template<class Char, class Traits>
inline bool operator==(const async_pipe & lhs, const basic_pipe<Char, Traits> & rhs)
{
    return compare_handles(lhs.native_source(), rhs.native_source()) &&
           compare_handles(lhs.native_sink(),   rhs.native_sink());
}

template<class Char, class Traits>
inline bool operator!=(const async_pipe & lhs, const basic_pipe<Char, Traits> & rhs)
{
    return !compare_handles(lhs.native_source(), rhs.native_source()) ||
           !compare_handles(lhs.native_sink(),   rhs.native_sink());
}

template<class Char, class Traits>
inline bool operator==(const basic_pipe<Char, Traits> & lhs, const async_pipe & rhs)
{
    return compare_handles(lhs.native_source(), rhs.native_source()) &&
           compare_handles(lhs.native_sink(),   rhs.native_sink());
}

template<class Char, class Traits>
inline bool operator!=(const basic_pipe<Char, Traits> & lhs, const async_pipe & rhs)
{
    return !compare_handles(lhs.native_source(), rhs.native_source()) ||
           !compare_handles(lhs.native_sink(),   rhs.native_sink());
}

}}}}

#endif /* INCLUDE_BOOST_PIPE_DETAIL_WINDOWS_ASYNC_PIPE_HPP_ */
