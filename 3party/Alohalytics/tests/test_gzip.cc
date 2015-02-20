#include "../src/gzip_wrapper.h"

#include <cstdlib>

int main(int, char **) {
  std::string data;
  for (int i = 0; i < 10000 + rand(); ++i) {
    data.push_back(rand());
  }
  const std::string gzipped = alohalytics::Gzip(data);
  const std::string ungzipped = alohalytics::Gunzip(gzipped);
  if (ungzipped != data) {
    std::cerr << "Gzip/gunzip test has failed" << std::endl;
    return -1;
  }
  std::cout << "Test has passed" << std::endl;
  return 0;
}
