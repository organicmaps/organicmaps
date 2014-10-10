#pragma once

#include <string>

#include "../DataStructures/QueryEdge.h"

namespace mapsme
{

struct EdgeLess
{
  bool operator () (QueryEdge::EdgeData const & e1, QueryEdge::EdgeData const & e2) const
  {

    if (e1.distance != e2.distance)
      return e1.distance < e2.distance;

    if (e1.shortcut != e2.shortcut)
      return e1.shortcut < e2.shortcut;

    if (e1.forward != e2.forward)
      return e1.forward < e2.forward;

    if (e1.backward != e2.backward)
      return e1.backward < e2.backward;

    if (e1.id != e2.id)
      return e1.id < e2.id;

    return false;
  }
};

class Converter
{


public:
  Converter();

  void run(const std::string & name);


};

}
