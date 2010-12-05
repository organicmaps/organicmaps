#pragma once

#include <string>

class OnTestFailImpl;

class OnTestFail
{
    OnTestFailImpl * m_pImpl;
public:
    OnTestFail(bool ok, char const * check, char const * file, int line);
    void operator () (std::wstring const & s) const;
};
