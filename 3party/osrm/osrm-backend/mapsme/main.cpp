#include <stdlib.h>

#include <iostream>
#include <boost/program_options.hpp>
#include "converter.hpp"

int main(int argc, char **argv)
{

  boost::program_options::options_description descr;

  descr.add_options()
      ("help", "h")
      ("input,i", boost::program_options::value<std::string>(), "Path to input osrm file (belarus.osrm)");

  boost::program_options::variables_map vm;
  boost::program_options::store(boost::program_options::command_line_parser(argc, argv).options(descr).run(), vm);
  boost::program_options::notify(vm);
  if (vm.count("help") || !vm.count("input"))
  {
    std::cout << descr;
    return 0;
  }


  mapsme::GenerateRoutingIndex(vm["input"].as<std::string>());

  return 0;
}
