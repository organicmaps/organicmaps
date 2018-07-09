// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_PROCESS_DETAIL_WINDOWS_BASIC_CMD_HPP_
#define BOOST_PROCESS_DETAIL_WINDOWS_BASIC_CMD_HPP_

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/process/shell.hpp>
#include <boost/process/detail/windows/handler.hpp>

#include <vector>
#include <string>
#include <iterator>


namespace boost
{
namespace process
{
namespace detail
{
namespace windows
{

inline std::string build_args(const std::string & exe, std::vector<std::string> && data)
{
    std::string st = exe;

    //put in quotes if it has spaces
    {
        boost::replace_all(st, "\"", "\\\"");

        auto it = std::find(st.begin(), st.end(), ' ');

        if (it != st.end())//contains spaces.
        {
            st.insert(st.begin(), '"');
            st += '"';
        }
    }

    for (auto & arg : data)
    {
        boost::replace_all(arg, "\"", "\\\"");

        auto it = std::find(arg.begin(), arg.end(), ' ');//contains space?
        if (it != arg.end())//ok, contains spaces.
        {
            //the first one is put directly onto the output,
            //because then I don't have to copy the whole string
            arg.insert(arg.begin(), '"');
            arg += '"'; //thats the post one.
        }

        if (!st.empty())//first one does not need a preceeding space
            st += ' ';

        st += arg;
    }
    return st;
}

inline std::wstring build_args(const std::wstring & exe, std::vector<std::wstring> && data)
{
    std::wstring st = exe;
    for (auto & arg : data)
    {
        boost::replace_all(arg, L"\"", L"\\\"");

        auto it = std::find(arg.begin(), arg.end(), ' ');//contains space?
        if (it != arg.end())//ok, contains spaces.
        {
            //the first one is put directly onto the output,
            //because then I don't have to copy the whole string
            arg.insert(arg.begin(), L'"');
            arg += L'"'; //thats the post one.
        }

        if (!st.empty())//first one does not need a preceeding space
            st += L' ';

        st += arg;
    }
    return st;
}

template<typename Char>
struct exe_cmd_init : handler_base_ext
{
    using value_type  = Char;
    using string_type = std::basic_string<value_type>;

    static const char*    c_arg(char)    { return "/c";}
    static const wchar_t* c_arg(wchar_t) { return L"/c";}

    exe_cmd_init(const string_type & exe, bool cmd_only = false)
                : exe(exe), args({}), cmd_only(cmd_only) {};
    exe_cmd_init(string_type && exe, bool cmd_only = false)
                : exe(std::move(exe)), args({}), cmd_only(cmd_only) {};

    exe_cmd_init(string_type && exe, std::vector<string_type> && args)
            : exe(std::move(exe)), args(build_args(this->exe, std::move(args))), cmd_only(false) {};
    template <class Executor>
    void on_setup(Executor& exec) const
    {

        if (cmd_only && args.empty())
            exec.cmd_line = exe.c_str();
        else
        {
            exec.exe = exe.c_str();
            exec.cmd_line = args.c_str();
        }
    }
    static exe_cmd_init<Char> exe_args(string_type && exe, std::vector<string_type> && args)
    {
        return exe_cmd_init<Char>(std::move(exe), std::move(args));
    }
    static exe_cmd_init<Char> cmd(string_type&& cmd)
    {
        return exe_cmd_init<Char>(std::move(cmd), true);
    }
    static exe_cmd_init<Char> exe_args_shell(string_type && exe, std::vector<string_type> && args)
    {
        std::vector<string_type> args_ = {c_arg(Char()), std::move(exe)};
        args_.insert(args_.end(), std::make_move_iterator(args.begin()), std::make_move_iterator(args.end()));
        string_type sh = get_shell(Char());

        return exe_cmd_init<Char>(std::move(sh), std::move(args_));
    }

    static std:: string get_shell(char)    {return shell(). string(codecvt()); }
    static std::wstring get_shell(wchar_t) {return shell().wstring(codecvt());}

    static exe_cmd_init<Char> cmd_shell(string_type&& cmd)
    {
        std::vector<string_type> args = {c_arg(Char()), std::move(cmd)};
        string_type sh = get_shell(Char());

        return exe_cmd_init<Char>(
                std::move(sh),
                std::move(args));
    }
private:
    string_type exe;
    string_type args;
    bool cmd_only;
};

}



}
}
}



#endif /* INCLUDE_BOOST_PROCESS_WINDOWS_ARGS_HPP_ */
