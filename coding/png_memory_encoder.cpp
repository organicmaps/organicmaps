#include "png_memory_encoder.hpp"
#include "lodepng.hpp"

#include "base/assert.hpp"

namespace il
{

void EncodePngToMemory(size_t width, size_t height,
                       vector<uint8_t> const & rgba,
                       vector<uint8_t> & out)
{
  CHECK(LodePNG::encode(out, rgba, width, height) == 0, ());
  CHECK(out.size() != 0, ());
}

} // namespace il
