/*

  Example program to look at Osmium indexes on disk.

  The code in this example file is released into the Public Domain.

*/

#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>

#include <boost/program_options.hpp>

#include <osmium/index/map/dense_file_array.hpp>
#include <osmium/index/map/sparse_file_array.hpp>
#include <osmium/osm/location.hpp>
#include <osmium/osm/types.hpp>

template <typename TKey, typename TValue>
class IndexSearch {

    typedef typename osmium::index::map::DenseFileArray<TKey, TValue> dense_index_type;
    typedef typename osmium::index::map::SparseFileArray<TKey, TValue> sparse_index_type;

    int m_fd;
    bool m_dense_format;

    void dump_dense() {
        dense_index_type index(m_fd);

        for (size_t i = 0; i < index.size(); ++i) {
            if (index.get(i) != TValue()) {
                std::cout << i << " " << index.get(i) << "\n";
            }
        }
    }

    void dump_sparse() {
        sparse_index_type index(m_fd);

        for (auto& element : index) {
            std::cout << element.first << " " << element.second << "\n";
        }
    }

    bool search_dense(TKey key) {
        dense_index_type index(m_fd);

        try {
            TValue value = index.get(key);
            std::cout << key << " " << value << std::endl;
        } catch (...) {
            std::cout << key << " not found" << std::endl;
            return false;
        }

        return true;
    }

    bool search_sparse(TKey key) {
        typedef typename sparse_index_type::element_type element_type;
        sparse_index_type index(m_fd);

        element_type elem {key, TValue()};
        auto positions = std::equal_range(index.begin(), index.end(), elem, [](const element_type& lhs, const element_type& rhs) {
            return lhs.first < rhs.first;
        });
        if (positions.first == positions.second) {
            std::cout << key << " not found" << std::endl;
            return false;
        }

        for (auto& it = positions.first; it != positions.second; ++it) {
            std::cout << it->first << " " << it->second << "\n";
        }

        return true;
    }

public:

    IndexSearch(int fd, bool dense_format) :
        m_fd(fd),
        m_dense_format(dense_format) {
    }

    void dump() {
        if (m_dense_format) {
            dump_dense();
        } else {
            dump_sparse();
        }
    }

    bool search(TKey key) {
        if (m_dense_format) {
            return search_dense(key);
        } else {
            return search_sparse(key);
        }
    }

    bool search(std::vector<TKey> keys) {
        bool found_all = true;

        for (const auto key : keys) {
            if (!search(key)) {
                found_all = false;
            }
        }

        return found_all;
    }

}; // class IndexSearch

enum return_code : int {
    okay      = 0,
    not_found = 1,
    error     = 2,
    fatal     = 3
};

namespace po = boost::program_options;

class Options {

    po::variables_map vm;

public:

    Options(int argc, char* argv[]) {
        try {
            po::options_description desc("Allowed options");
            desc.add_options()
                ("help,h", "Print this help message")
                ("array,a", po::value<std::string>(), "Read given index file in array format")
                ("list,l", po::value<std::string>(), "Read given index file in list format")
                ("dump,d", "Dump contents of index file to STDOUT")
                ("search,s", po::value<std::vector<osmium::unsigned_object_id_type>>(), "Search for given id (Option can appear multiple times)")
                ("type,t", po::value<std::string>(), "Type of value ('location' or 'offset')")
            ;

            po::store(po::parse_command_line(argc, argv, desc), vm);
            po::notify(vm);

            if (vm.count("help")) {
                std::cout << desc << "\n";
                exit(return_code::okay);
            }

            if (vm.count("array") && vm.count("list")) {
                std::cerr << "Only option --array or --list allowed." << std::endl;
                exit(return_code::fatal);
            }

            if (!vm.count("array") && !vm.count("list")) {
                std::cerr << "Need one of option --array or --list." << std::endl;
                exit(return_code::fatal);
            }

            if (!vm.count("type")) {
                std::cerr << "Need --type argument." << std::endl;
                exit(return_code::fatal);
            }

            const std::string& type = vm["type"].as<std::string>();
            if (type != "location" && type != "offset") {
                std::cerr << "Unknown type '" << type << "'. Must be 'location' or 'offset'." << std::endl;
                exit(return_code::fatal);
            }
        } catch (boost::program_options::error& e) {
            std::cerr << "Error parsing command line: " << e.what() << std::endl;
            exit(return_code::fatal);
        }
    }

    const std::string& filename() const {
        if (vm.count("array")) {
            return vm["array"].as<std::string>();
        } else {
            return vm["list"].as<std::string>();
        }
    }

    bool dense_format() const {
        return vm.count("array") != 0;
    }

    bool do_dump() const {
        return vm.count("dump") != 0;
    }

    std::vector<osmium::unsigned_object_id_type> search_keys() const {
        return vm["search"].as<std::vector<osmium::unsigned_object_id_type>>();
    }

    bool type_is(const char* type) const {
        return vm["type"].as<std::string>() == type;
    }

}; // class Options

int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);

    Options options(argc, argv);

    std::cout << std::fixed << std::setprecision(7);
    int fd = open(options.filename().c_str(), O_RDWR);

    bool result_okay = true;

    if (options.type_is("location")) {
        IndexSearch<osmium::unsigned_object_id_type, osmium::Location> is(fd, options.dense_format());

        if (options.do_dump()) {
            is.dump();
        } else {
            result_okay = is.search(options.search_keys());
        }
    } else {
        IndexSearch<osmium::unsigned_object_id_type, size_t> is(fd, options.dense_format());

        if (options.do_dump()) {
            is.dump();
        } else {
            result_okay = is.search(options.search_keys());
        }
    }

    exit(result_okay ? return_code::okay : return_code::not_found);
}

