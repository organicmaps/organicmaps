#ifndef EXTRACTOR_H_
#define EXTRACTOR_H_

#include <boost/filesystem.hpp>

#include <string>

class ExtractorCallbacks;

/** \brief Class of 'extract' utility. */
class Extractor
{
  protected:
    unsigned requested_num_threads;
    boost::filesystem::path config_file_path;
    boost::filesystem::path input_path;
    boost::filesystem::path profile_path;

    std::string output_file_name;
    std::string restriction_file_name;
    bool file_has_pbf_format;

    /** \brief Parses "extractor's" command line arguments */
    bool ParseArguments(int argc, char *argv[]);

    /** \brief Parses config file, if present in options */
    void GenerateOutputFilesNames();

  public:
    explicit Extractor();
    Extractor(const Extractor &) = delete;
    virtual ~Extractor();

    int Run(int argc, char *argv[]);
};
#endif /* EXTRACTOR_H_ */
