#include "pugixml.hpp"

#include <string>
#include <iostream>
#include <cstring>

// tag::code[]
struct xml_string_writer: pugi::xml_writer
{
    std::string result;

    virtual void write(const void* data, size_t size)
    {
        result.append(static_cast<const char*>(data), size);
    }
};
// end::code[]

struct xml_memory_writer: pugi::xml_writer
{
    char* buffer;
    size_t capacity;

    size_t result;

    xml_memory_writer(): buffer(0), capacity(0), result(0)
    {
    }

    xml_memory_writer(char* buffer, size_t capacity): buffer(buffer), capacity(capacity), result(0)
    {
    }

    size_t written_size() const
    {
        return result < capacity ? result : capacity;
    }

    virtual void write(const void* data, size_t size)
    {
        if (result < capacity)
        {
            size_t chunk = (capacity - result < size) ? capacity - result : size;

            memcpy(buffer + result, data, chunk);
        }

        result += size;
    }
};

std::string node_to_string(pugi::xml_node node)
{
    xml_string_writer writer;
    node.print(writer);

    return writer.result;
}

char* node_to_buffer(pugi::xml_node node, char* buffer, size_t size)
{
    if (size == 0) return buffer;

    // leave one character for null terminator
    xml_memory_writer writer(buffer, size - 1);
    node.print(writer);

    // null terminate
    buffer[writer.written_size()] = 0;

    return buffer;
}

char* node_to_buffer_heap(pugi::xml_node node)
{
    // first pass: get required memory size
    xml_memory_writer counter;
    node.print(counter);

    // allocate necessary size (+1 for null termination)
    char* buffer = new char[counter.result + 1];

    // second pass: actual printing
    xml_memory_writer writer(buffer, counter.result);
    node.print(writer);

    // null terminate
    buffer[writer.written_size()] = 0;

    return buffer;
}

int main()
{
    // get a test document
    pugi::xml_document doc;
    doc.load_string("<foo bar='baz'>hey</foo>");

    // get contents as std::string (single pass)
    std::cout << "contents: [" << node_to_string(doc) << "]\n";

    // get contents into fixed-size buffer (single pass)
    char large_buf[128];
    std::cout << "contents: [" << node_to_buffer(doc, large_buf, sizeof(large_buf)) << "]\n";

    // get contents into fixed-size buffer (single pass, shows truncating behavior)
    char small_buf[22];
    std::cout << "contents: [" << node_to_buffer(doc, small_buf, sizeof(small_buf)) << "]\n";

    // get contents into heap-allocated buffer (two passes)
    char* heap_buf = node_to_buffer_heap(doc);
    std::cout << "contents: [" << heap_buf << "]\n";
    delete[] heap_buf;
}

// vim:et
