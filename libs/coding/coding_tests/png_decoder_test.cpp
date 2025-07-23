#include "testing/testing.hpp"

#include <fstream>
#include <string>
#include <vector>

using namespace std;

void loadFile(vector<unsigned char> & buffer,
              string const & filename)  // designed for loading files from hard disk in an std::vector
{
  ifstream file(filename.c_str(), ios::in | ios::binary | ios::ate);

  // get filesize
  streamsize size = 0;
  if (file.seekg(0, ios::end).good())
    size = file.tellg();
  if (file.seekg(0, ios::beg).good())
    size -= file.tellg();

  // read contents of the file into the vector
  if (size > 0)
  {
    buffer.resize((size_t)size);
    file.read((char *)(&buffer[0]), size);
  }
  else
    buffer.clear();
}

UNIT_TEST(PngDecode)
{
  //  // load and decode
  //  vector<unsigned char> buffer, image;
  //  loadFile(buffer, "../../data/font_0.png");
  //  unsigned long w, h;
  //  int error = DecodePNG(image, w, h, buffer.empty() ? 0 : &buffer[0], (unsigned long)buffer.size());
  //
  //  // if there's an error, display it
  //  TEST_EQUAL(error, 0, ());
  //  // the pixels are now in the vector "image", use it as texture, draw it, ...
  //  TEST_GREATER(image.size(), 4, ("Image is empty???"));
  //  TEST_EQUAL(w, 1024, ());
  //  TEST_EQUAL(h, 1024, ());
}
