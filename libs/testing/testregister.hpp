#pragma once

#include <functional>
#include <utility>

class TestRegister
{
public:
  TestRegister(char const * testname, char const * filename, std::function<void()> && fnTest)
    : m_testname(testname)
    , m_filename(filename)
    , m_fn(std::move(fnTest))
    , m_next(nullptr)
  {
    if (FirstRegister() == nullptr)
    {
      FirstRegister() = this;
    }
    else
    {
      TestRegister * last = FirstRegister();
      while (last->m_next != nullptr)
        last = last->m_next;
      last->m_next = this;
    }
  }

  static TestRegister *& FirstRegister()
  {
    static TestRegister * test = nullptr;
    return test;
  }

  // Test name.
  char const * m_testname;

  // File name.
  char const * m_filename;

  // Function to run test.
  std::function<void()> m_fn;

  // Next test in chain.
  TestRegister * m_next;
};
