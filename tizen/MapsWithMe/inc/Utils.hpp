#pragma once
#include <FBase.h>

Tizen::Base::String GetString(const wchar_t * IDC);
Tizen::Base::String FormatString1(const wchar_t * IDC, Tizen::Base::String const & param1);
Tizen::Base::String FormatString2(const wchar_t * IDC, Tizen::Base::String const & param1, Tizen::Base::String const & param2);
bool MessageBoxAsk(Tizen::Base::String const & title, Tizen::Base::String const & msg);
void MessageBoxOk(Tizen::Base::String const & title, Tizen::Base::String const & msg);
