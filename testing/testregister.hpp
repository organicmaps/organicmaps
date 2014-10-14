#pragma once


class TestRegister
{
public:
  TestRegister(char const * testName, char const * fileName, void (* fnTest)())
      : m_TestName(testName), m_FileName(fileName), m_Fn(fnTest), m_pNext(0)
  {
    if (FirstRegister() == 0)
      FirstRegister() = this;
    else
    {
      TestRegister * pLast = FirstRegister();
      while (pLast->m_pNext)
        pLast = pLast->m_pNext;
      pLast->m_pNext = this;
    }
  }

  // Test name.
  char const * m_TestName;
  // File name.
  char const * m_FileName;
  // Function to run test.
  void (* m_Fn)();
  // Next test in chain.
  TestRegister * m_pNext;

  static TestRegister * & FirstRegister()
  {
    static TestRegister * s_pRegister = 0;
    return s_pRegister;
  }
};
