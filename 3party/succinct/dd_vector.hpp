#pragma once

#include <cassert>
#include <cstdint>
#include <utility>

namespace coding
{
template <typename TReader>
class ReaderMapVisitor;
}

namespace succinct { namespace mapper {

    // A reader of memory says so with a static constant kIsMemoryReader = true: its Read() is
    // cheap and inline. Any other reader, like the one of a file, pays for a call much more than
    // for the bytes that it reads.
    template <class TReader>
    constexpr bool is_memory_reader = requires { requires TReader::kIsMemoryReader; };

    namespace detail {
        // Where the elements of a vector are in the reader.
        template <typename T, class TReader>
        class dd_vector_base
        {
        public:
            typedef T value_type;

            uint64_t size() const {
                return m_size;
            }

            friend class coding::ReaderMapVisitor<TReader>;

        protected:
            dd_vector_base()
                : m_reader(nullptr)
                , m_offset(0)
                , m_size(0)
            {}

            void swap(dd_vector_base& other) {
                using std::swap;
                swap(m_reader, other.m_reader);
                swap(m_offset, other.m_offset);
                swap(m_size, other.m_size);
            }

            // Reads |count| elements from the element |first|.
            void read(uint64_t first, T* to, uint64_t count) const {
                m_reader->Read(m_offset + sizeof(T) * first, to, sizeof(T) * count);
            }

            TReader const* m_reader;
            uint64_t m_offset;
            uint64_t m_size;
        };
    }

    // "Disk driven" counterpart of mappable_vector: the elements are read from a reader, which
    // needs Read(uint64_t pos, void * p, size_t size) const and has to outlive the vector.
    template <typename T, class TReader, bool in_memory = is_memory_reader<TReader>> // T must be a POD
    class dd_vector;

    // The vector of a reader of memory reads an element when it is asked for, so it is as fast
    // as the memory of the reader and keeps nothing.
    template <typename T, class TReader>
    class dd_vector<T, TReader, true> : public detail::dd_vector_base<T, TReader>
    {
        typedef detail::dd_vector_base<T, TReader> base_type;

    public:
        // The same kind of vector for another element type.
        template <typename TT> using UnderlyingT = dd_vector<TT, TReader>;

        dd_vector() = default;
        dd_vector(dd_vector const &) = delete;
        dd_vector & operator=(dd_vector const &) = delete;
        dd_vector(dd_vector &&) = default;
        dd_vector & operator=(dd_vector &&) = default;

        void swap(dd_vector& other) {
            base_type::swap(other);
        }

        void clear() {
            dd_vector().swap(*this);
        }

        inline T operator[](uint64_t i) const {
            assert(i < this->m_size);
            T value;
            this->read(i, &value, 1);
            return value;
        }
    };

    // The vector of any other reader keeps a window of the last read elements: a succinct
    // structure asks for the neighbours of an element next, and one read of a window is much
    // cheaper than a read for every element. Such a vector is not thread safe, like its reader.
    template <typename T, class TReader>
    class dd_vector<T, TReader, false> : public detail::dd_vector_base<T, TReader>
    {
        typedef detail::dd_vector_base<T, TReader> base_type;

    public:
        // The same kind of vector for another element type.
        template <typename TT> using UnderlyingT = dd_vector<TT, TReader>;

        // The number of the elements in the window, a power of two. A window is missed when the
        // next element is far from the last one, and a bigger window costs more to read then.
        static constexpr uint64_t window_size = 8;

        dd_vector() = default;
        dd_vector(dd_vector const &) = delete;
        dd_vector & operator=(dd_vector const &) = delete;
        dd_vector(dd_vector &&) = default;
        dd_vector & operator=(dd_vector &&) = default;

        void swap(dd_vector& other) {
            base_type::swap(other);
            // The windows are of other elements now.
            m_window_first = other.m_window_first = no_window;
        }

        void clear() {
            dd_vector().swap(*this);
        }

        inline T operator[](uint64_t i) const {
            assert(i < this->m_size);
            uint64_t const first = i & ~(window_size - 1);
            if (first != m_window_first) {
                uint64_t const left = this->m_size - first;
                this->read(first, m_window, left < window_size ? left : window_size);
                m_window_first = first;
            }
            return m_window[i - first];
        }

    private:
        static constexpr uint64_t no_window = uint64_t(-1);

        mutable T m_window[window_size] = {};
        mutable uint64_t m_window_first = no_window;
    };

}}
