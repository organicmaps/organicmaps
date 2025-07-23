#pragma once

#include "base/assert.hpp"

#include <iostream>
#include <string>

namespace tree
{
template <class ToDo>
bool LoadTreeAsText(std::istream & s, ToDo & toDo)
{
  std::string name;
  s >> name;
  ASSERT(!name.empty(), ("Error in classificator file"));
  if (name == "{}")
    return false;

  // set key name
  toDo.Name(name);

  // load object itself
  std::string strkey;
  s >> strkey;
  while (strkey != "+" && strkey != "-")
    s >> strkey;

  // load children
  if (strkey == "+")
  {
    size_t i = 0;
    while (true)
    {
      toDo.Start(i++);
      bool const isContinue = LoadTreeAsText(s, toDo);
      toDo.End();

      if (!isContinue)
      {
        toDo.EndChilds();
        break;
      }
    }

    ASSERT(i <= 128, ("too many features at level = ", name));
  }

  return true;
}
}  // namespace tree
