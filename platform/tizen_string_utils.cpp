#include "tizen_string_utils.hpp"
#include "../../std/vector.hpp"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wignored-qualifiers"
  #include <FBase.h>
#pragma clang diagnostic pop

string FromTizenString(Tizen::Base::String const & str_tizen)
{
  string utf8Str;
  if (str_tizen.GetLength() == 0)
    return utf8Str;
  Tizen::Base::ByteBuffer * pBuffer = Tizen::Base::Utility::StringUtil::StringToUtf8N(str_tizen);
  if (pBuffer)
  {
    int byteCount = pBuffer->GetLimit();

    byteCount--; // Don't copy Zero at the end
    if (byteCount > 0)
    {
      vector<char> chBuf(byteCount);
      pBuffer->GetArray((byte *)&chBuf[0], 0, byteCount);
      utf8Str.assign(chBuf.begin(), chBuf.end());
    }
    delete pBuffer;
  }
  return utf8Str;
}
