#pragma once

#include <string>
#include <stdexcept>
#include <fstream>
#include <list>

#include <stdint.h>

#include <boost/iterator/iterator_facade.hpp>

namespace succinct { namespace util {

    inline void trim_newline_chars(std::string& s)
    {
        size_t l = s.size();
        while (l && (s[l-1] == '\r' ||
                     s[l-1] == '\n')) {
            --l;
        }
        s.resize(l);
    }

    // this is considerably faster than std::getline
    inline bool fast_getline(std::string& line, FILE* input = stdin, bool trim_newline = false)
    {
        line.clear();
        static const size_t max_buffer = 65536;
        char buffer[max_buffer];
        bool done = false;
        while (!done) {
            if (!fgets(buffer, max_buffer, input)) {
                if (!line.size()) {
                    return false;
                } else {
                    done = true;
                }
            }
            line += buffer;
            if (*line.rbegin() == '\n') {
                done = true;
            }
        }
        if (trim_newline) {
            trim_newline_chars(line);
        }
        return true;
    }

    class line_iterator
        : public boost::iterator_facade<line_iterator
                                        , std::string const
                                        , boost::forward_traversal_tag
                                        >
    {
    public:
        line_iterator()
            : m_file(0)
        {}

        explicit line_iterator(FILE* input, bool trim_newline = false)
            : m_file(input)
            , m_trim_newline(trim_newline)
        {}

    private:
        friend class boost::iterator_core_access;

        void increment() {
            assert(m_file);
            if (!fast_getline(m_line, m_file, m_trim_newline)) {
                m_file = 0;
            }
        }

        bool equal(line_iterator const& other) const
        {
            return this->m_file == other.m_file;
        }

        std::string const& dereference() const {
            return m_line;
        }

        std::string m_line;
        FILE* m_file;
        bool m_trim_newline;
    };

    typedef std::pair<line_iterator, line_iterator> line_range_t;

    inline line_range_t lines(FILE* ifs, bool trim_newline = false) {
        return std::make_pair(line_iterator(ifs, trim_newline), line_iterator());
    }

    struct auto_file {

        auto_file(const char* name, const char* mode = "rb")
            : m_file(0)
        {
            m_file = fopen(name, mode);
            if(!m_file) {
                std::string msg("Unable to open file '");
                msg += name;
                msg += "'.";
                throw std::invalid_argument(msg);

            }
        }

        ~auto_file()
        {
            if(m_file) {
                fclose(m_file);
            }
        }

        FILE* get()
        {
            return m_file;
        }

    private:
        auto_file();
        auto_file( const auto_file & );
        auto_file & operator=( const auto_file & );

        FILE * m_file;
    };

    typedef std::pair<const uint8_t*, const uint8_t*> char_range;

    struct identity_adaptor
    {
        char_range operator()(char_range s) const
        {
            return s;
        }
    };

    struct stl_string_adaptor
    {
        char_range operator()(std::string const& s) const
        {
            const uint8_t* buf = reinterpret_cast<const uint8_t*>(s.c_str());
            const uint8_t* end = buf + s.size() + 1; // add the null terminator
            return char_range(buf, end);
        }
    };

    class buffer_line_iterator
        : public boost::iterator_facade<buffer_line_iterator
                                        , std::string const
                                        , boost::forward_traversal_tag
                                        >
    {
    public:
        buffer_line_iterator()
            : m_end(0)
            , m_cur_pos(0)
        {}

        buffer_line_iterator(const char* buffer, size_t size)
            :m_end(buffer + size)
            , m_cur_pos(buffer)
        {
            increment();
        }

    private:
        friend class boost::iterator_core_access;

        void increment() {
            assert(m_cur_pos);
            if (m_cur_pos >= m_end) {
                m_cur_pos = 0;
                return;
            }
            const char* begin = m_cur_pos;
            while (m_cur_pos < m_end && *m_cur_pos != '\n') {
                ++m_cur_pos;
            }
            const char* end = m_cur_pos;
            ++m_cur_pos; // skip the newline

            if (begin != end && *(end - 1) == '\r') {
                --end;
            }
            m_cur_value = std::string(begin, size_t(end - begin));
        }

        bool equal(buffer_line_iterator const& other) const
        {
            return m_cur_pos == other.m_cur_pos;        }

        std::string const& dereference() const
        {
            assert(m_cur_pos);
            return m_cur_value;
        }

        const char* m_end;
        const char* m_cur_pos;
        std::string m_cur_value;
    };

    struct input_error : std::invalid_argument
    {
        input_error(std::string const& what)
            : invalid_argument(what)
        {}
    };

    template <typename T>
    inline void dispose(T& t)
    {
        T().swap(t);
    }

    inline uint64_t int2nat(int64_t x)
    {
        if (x < 0) {
            return uint64_t(-2 * x - 1);
        } else {
            return uint64_t(2 * x);
        }
    }

    inline int64_t nat2int(uint64_t n)
    {
        if (n % 2) {
            return -int64_t((n + 1) / 2);
        } else {
            return int64_t(n / 2);
        }
    }

    template <typename IntType1, typename IntType2>
    inline IntType1 ceil_div(IntType1 dividend, IntType2 divisor)
    {
        // XXX(ot): put some static check that IntType1 >= IntType2
        IntType1 d = IntType1(divisor);
        return IntType1(dividend + d - 1) / d;
    }

}}
