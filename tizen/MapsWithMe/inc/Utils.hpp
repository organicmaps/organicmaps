#pragma once
#include <FBase.h>

namespace Tizen
{
namespace Graphics
{
class Bitmap;
} // namespace Graphics
} // namespace Tizen

class Framework;
class UserMarkCopy;

Tizen::Base::String GetString(const wchar_t * IDC);
Tizen::Base::String FormatString1(const wchar_t * IDC, Tizen::Base::String const & param1);
Tizen::Base::String FormatString2(const wchar_t * IDC, Tizen::Base::String const & param1, Tizen::Base::String const & param2);
bool MessageBoxAsk(Tizen::Base::String const & title, Tizen::Base::String const & msg);
void MessageBoxOk(Tizen::Base::String const & title, Tizen::Base::String const & msg);
Tizen::Graphics::Bitmap const * GetBitmap(const wchar_t * sBitmapPath);
::Framework * GetFramework();

Tizen::Base::String GetFeature(Tizen::Base::String const & feature);
