#pragma once

#include <vector>
#include <algorithm>

#include <boost/utility.hpp>
#include <boost/range.hpp>
#include <boost/function.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/construct.hpp>

#include <stdint.h>

#include "intrinsics.hpp"

namespace coding
{
template<typename TWriter>
class FreezeVisitor;

template <typename TWriter>
class ReverseFreezeVisitor;

class MapVisitor;
class ReverseMapVisitor;
}

namespace succinct { namespace mapper {

    namespace detail {
        class freeze_visitor;
        class map_visitor;
        class sizeof_visitor;
    }

    typedef boost::function<void()> deleter_t;

    template <typename T> // T must be a POD
    class mappable_vector : boost::noncopyable {
    public:
        typedef T value_type;
        typedef const T* iterator;
        typedef const T* const_iterator;

        mappable_vector()
            : m_data(0)
            , m_size(0)
            , m_deleter()
        {}

        template <typename Range>
        mappable_vector(Range const& from)
            : m_data(0)
            , m_size(0)
        {
            size_t size = boost::size(from);
            T* data = new T[size];
            m_deleter = boost::lambda::bind(boost::lambda::delete_array(), data);

            std::copy(boost::begin(from),
                      boost::end(from),
                      data);
            m_data = data;
            m_size = size;
        }

        ~mappable_vector() {
            if (m_deleter) {
                m_deleter();
            }
        }

        void swap(mappable_vector& other) {
            using std::swap;
            swap(m_data, other.m_data);
            swap(m_size, other.m_size);
            swap(m_deleter, other.m_deleter);
        }

        void clear() {
            mappable_vector().swap(*this);
        }

        void steal(std::vector<T>& vec) {
            clear();
            m_size = vec.size();
            if (m_size) {
                std::vector<T>* new_vec = new std::vector<T>;
                new_vec->swap(vec);
                m_deleter = boost::lambda::bind(boost::lambda::delete_ptr(), new_vec);
                m_data = &(*new_vec)[0];
            }
        }

        template <typename Range>
        void assign(Range const& from) {
            clear();
            mappable_vector(from).swap(*this);
        }

        uint64_t size() const {
            return m_size;
        }

        inline const_iterator begin() const {
            return m_data;
        }

        inline const_iterator end() const {
            return m_data + m_size;
        }

        inline T const& operator[](uint64_t i) const {
            assert(i < m_size);
            return m_data[i];
        }

        inline T const* data() const {
            return m_data;
        }

        inline void prefetch(size_t i) const {
            succinct::intrinsics::prefetch(m_data + i);
        }

        friend class detail::freeze_visitor;
        friend class detail::map_visitor;
        friend class detail::sizeof_visitor;

        template<typename TWriter>
        friend class coding::FreezeVisitor;

        template<typename TWriter>
        friend class coding::ReverseFreezeVisitor;

        friend class coding::MapVisitor;
        friend class coding::ReverseMapVisitor;

    protected:
        const T* m_data;
        uint64_t m_size;
        deleter_t m_deleter;
    };

}}
