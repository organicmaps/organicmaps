#pragma once

#include <iostream>
#include <fstream>

#include <boost/make_shared.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/utility.hpp>
#include <boost/type_traits/is_pod.hpp>

#include "mappable_vector.hpp"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#endif

namespace succinct { namespace mapper {

    #define NEED_TO_ALIGN4(v) (4 - ((v) % 4))
    #define ALIGN4_PTR(v) { uint32_t x = (uint64_t)(v) % 4; if (x > 0) (v) += 4 - x; }

    struct freeze_flags {
        // enum {
        // };
    };

    struct map_flags {
        enum {
            warmup = 1
        };
    };

    struct size_node;
    typedef boost::shared_ptr<size_node> size_node_ptr;

    struct size_node
    {
        size_node()
            : size(0)
        {}

        std::string name;
        uint64_t size;
        std::vector<size_node_ptr> children;

        void dump(std::ostream& os = std::cerr, size_t depth = 0) {
            os << std::string(depth * 4, ' ')
               << name << ": "
               << size << '\n';
            for (size_t i = 0; i < children.size(); ++i) {
                children[i]->dump(os, depth + 1);
            }
        }
    };

    namespace detail {
        class freeze_visitor : boost::noncopyable {
        public:
            freeze_visitor(std::ofstream& fout, uint64_t flags)
                : m_fout(fout)
                , m_flags(flags)
                , m_written(0)
            {
                // Save freezing flags
                m_fout.write(reinterpret_cast<const char*>(&m_flags), sizeof(m_flags));
                m_written += sizeof(m_flags);
            }

            template <typename T>
            typename boost::disable_if<boost::is_pod<T>, freeze_visitor&>::type
            operator()(T& val, const char* /* friendly_name */) {
                val.map(*this);
                return *this;
            }

            template <typename T>
            typename boost::enable_if<boost::is_pod<T>, freeze_visitor&>::type
            operator()(T& val, const char* /* friendly_name */) {
                m_fout.write(reinterpret_cast<const char*>(&val), sizeof(T));
                m_written += sizeof(T);

                uint32_t padding = NEED_TO_ALIGN4(m_written);
                static uint32_t const zero = 0;
                if (padding > 0 && padding < 4)
                {
                  m_fout.write(reinterpret_cast<const char*>(&zero), padding);
                  m_written += padding;
                }
                return *this;
            }

            template<typename T>
            freeze_visitor&
            operator()(mappable_vector<T>& vec, const char* /* friendly_name */) {
                (*this)(vec.m_size, "size");

                size_t n_bytes = static_cast<size_t>(vec.m_size * sizeof(T));
                m_fout.write(reinterpret_cast<const char*>(vec.m_data), long(n_bytes));
                m_written += n_bytes;

                uint32_t padding = NEED_TO_ALIGN4(m_written);
                static uint32_t const zero = 0;
                if (padding > 0 && padding < 4)
                {
                  m_fout.write(reinterpret_cast<const char*>(&zero), padding);
                  m_written += padding;
                }

                return *this;
            }

            size_t written() const {
                return m_written;
            }

        protected:
            std::ofstream& m_fout;
            const uint64_t m_flags;
            uint64_t m_written;
        };

        class map_visitor : boost::noncopyable {
        public:
            map_visitor(const char* base_address, uint64_t flags)
                : m_base(base_address)
                , m_cur(m_base)
                , m_flags(flags)
            {
                m_freeze_flags = *reinterpret_cast<const uint64_t*>(m_cur);
                m_cur += sizeof(m_freeze_flags);
            }

            template <typename T>
            typename boost::disable_if<boost::is_pod<T>, map_visitor&>::type
            operator()(T& val, const char* /* friendly_name */) {
                val.map(*this);
                return *this;
            }

            template <typename T>
            typename boost::enable_if<boost::is_pod<T>, map_visitor&>::type
            operator()(T& val, const char* /* friendly_name */) {
                val = *reinterpret_cast<const T*>(m_cur);
                m_cur += sizeof(T);

                ALIGN4_PTR(m_cur);
                return *this;
            }

            template<typename T>
            map_visitor&
            operator()(mappable_vector<T>& vec, const char* /* friendly_name */) {
                vec.clear();
                (*this)(vec.m_size, "size");

                vec.m_data = reinterpret_cast<const T*>(m_cur);
                size_t bytes = vec.m_size * sizeof(T);

                if (m_flags & map_flags::warmup) {
                    T foo;
                    volatile T* bar = &foo;
                    for (size_t i = 0; i < vec.m_size; ++i) {
                        *bar = vec.m_data[i];
                    }
                }

                m_cur += bytes;
                ALIGN4_PTR(m_cur);
                return *this;
            }

            size_t bytes_read() const {
                return size_t(m_cur - m_base);
            }

        protected:
            const char* m_base;
            const char* m_cur;
            const uint64_t m_flags;
            uint64_t m_freeze_flags;
        };

        class sizeof_visitor : boost::noncopyable {
        public:
            sizeof_visitor(bool with_tree = false)
                : m_size(0)
            {
                if (with_tree) {
                    m_cur_size_node = boost::make_shared<size_node>();
                }
            }

            template <typename T>
            typename boost::disable_if<boost::is_pod<T>, sizeof_visitor&>::type
            operator()(T& val, const char* friendly_name) {
                uint64_t checkpoint = m_size;
                size_node_ptr parent_node;
                if (m_cur_size_node) {
                    parent_node = m_cur_size_node;
                    m_cur_size_node = make_node(friendly_name);
                }

                val.map(*this);

                if (m_cur_size_node) {
                    m_cur_size_node->size = m_size - checkpoint;
                    m_cur_size_node = parent_node;
                }
                return *this;
            }

            template <typename T>
            typename boost::enable_if<boost::is_pod<T>, sizeof_visitor&>::type
            operator()(T& /* val */, const char* /* friendly_name */) {
                // don't track PODs in the size tree (they are constant sized)
                m_size += sizeof(T);
                return *this;
            }

            template<typename T>
            sizeof_visitor&
            operator()(mappable_vector<T>& vec, const char* friendly_name) {
                uint64_t checkpoint = m_size;
                (*this)(vec.m_size, "size");
                m_size += static_cast<uint64_t>(vec.m_size * sizeof(T));

                if (m_cur_size_node) {
                    make_node(friendly_name)->size = m_size - checkpoint;
                }

                return *this;
            }

            uint64_t size() const {
                return m_size;
            }

            size_node_ptr size_tree() const {
                assert(m_cur_size_node);
                return m_cur_size_node;
            }

        protected:

            size_node_ptr make_node(const char* name)
            {
                size_node_ptr node = boost::make_shared<size_node>();
                m_cur_size_node->children.push_back(node);
                node->name = name;
                return node;
            }

            uint64_t m_size;
            size_node_ptr m_cur_size_node;
        };

    }

    template <typename T>
    size_t freeze(T& val, std::ofstream& fout, uint64_t flags = 0, const char* friendly_name = "<TOP>")
    {
        detail::freeze_visitor freezer(fout, flags);
        freezer(val, friendly_name);
        return freezer.written();
    }

    template <typename T>
    size_t freeze(T& val, const char* filename, uint64_t flags = 0, const char* friendly_name = "<TOP>")
    {
        std::ofstream fout;
        fout.exceptions(std::ifstream::failbit);
        fout.open(filename, std::ios::binary);
        return freeze(val, fout, flags, friendly_name);
    }

    template <typename T>
    size_t map(T& val, const char* base_address, uint64_t flags = 0, const char* friendly_name = "<TOP>")
    {
        detail::map_visitor mapper(base_address, flags);
        mapper(val, friendly_name);
        return mapper.bytes_read();
    }

    template <typename T>
    uint64_t size_of(T& val)
    {
        detail::sizeof_visitor sizer;
        sizer(val, "");
        return sizer.size();
    }

    template <typename T>
    size_node_ptr size_tree_of(T& val, const char* friendly_name = "<TOP>")
    {
        detail::sizeof_visitor sizer(true);
        sizer(val, friendly_name);
        assert(sizer.size_tree()->children.size());
        return sizer.size_tree()->children[0];
    }

}}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
