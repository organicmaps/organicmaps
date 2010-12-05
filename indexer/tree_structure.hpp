#pragma once

#include "../base/assert.hpp"

#include "../std/fstream.hpp"
#include "../std/sstream.hpp"

namespace tree
{
  namespace detail
  {
    void PrintOffset(size_t offset, ostream & s)
    {
      for (size_t i = 0; i < offset; ++i)
        s << " ";
    }

    template <class ToDo>
    void PrintTextTree(size_t offset, ostream & s, ToDo & toDo)
    {
      PrintOffset(offset, s);

      // print name as key
      s << toDo.Name() << "  ";

      // serialize object
      toDo.Serialize(s);

      size_t const count = toDo.BeginChilds();
      bool const isEmpty = (count == 0);

      // put end marker
      s << (isEmpty ? "-" : "+") << endl;

      // print chils
      if (!isEmpty)
      {
        offset += 4;

        size_t i = 0;
        while (i < count)
        {
          toDo.Start(i++);
          PrintTextTree(offset, s, toDo);
          toDo.End();
        }

        // end of structure
        PrintOffset(offset, s);
        s << "{}" << endl;
      }
    }
  }

  template <class ToDo>
  void SaveTreeAsText(ostream & s, ToDo & toDo)
  {
    detail::PrintTextTree(0, s, toDo);
  }

  template <class ToDo>
  bool LoadTreeAsText(istringstream & s, ToDo & toDo)
  {
    string name;
    s >> name;
    ASSERT ( !name.empty(), ("Error in classificator file") );
    if (name == "{}") return false;

    // set key name
    toDo.Name(name);

    // load object itself
    string strkey;
    s >> strkey;
    while (strkey != "+" && strkey != "-")
    {
      toDo.Serialize(strkey);
      s >> strkey;
    }

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

      ASSERT ( i <= 64, ("too many features at level = ", name) );
    }

    return true;
  }
}
