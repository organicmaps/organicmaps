#pragma once

#include "testing/testing.hpp"
#include "coding/coder.hpp"

// TODO: void CoderRandomTest(Coder & encoder, Coder & decoder);
// TODO: void CoderTextTest(Coder & encoder, Coder & decoder);
// TODO: Test buffer overrun coder behavior.

template <typename EncoderT, typename DecoderT>
void CoderAaaaTest(EncoderT encoder, DecoderT decoder)
{
  for (int i = 1; i <= 65536; (i > 16 && (i & 3) == 1) ? i = (i & ~3) * 2 : ++i)
  {
    string data(i, 'a'), encoded, decoded;
    encoder(&data[0], data.size(), encoded);
    decoder(&encoded[0], encoded.size(), decoded);
    TEST_EQUAL(data, decoded, ());
  }
}
