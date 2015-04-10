#include "storage/countries.hpp"
#include "storage/simple_tree.hpp"

#include "base/string_utils.hpp"

#include "geometry/cellid.hpp"

#include "std/iostream.hpp"

typedef m2::CellId<9> CountryCellId;

namespace mapinfo
{
  void CountryTreeNode::AddCellId(string const & cellId)
  {
    CountryCellId id = CountryCellId::FromString(cellId);
    m_ids.push_back(static_cast<uint16_t>(id.ToBitsAndLevel().first));
    ASSERT_EQUAL(CountryCellId::FromBitsAndLevel(m_ids.back(), 8).ToString(), cellId, ());
  }

  bool LoadCountries(SimpleTree<CountryTreeNode> & tree, std::istream & stream)
  {
    std::string line;
    CountryTreeNode * value = &tree.Value();
    while (stream.good())
    {
      std::getline(stream, line);
      if (line.empty())
        continue;

      // calculate spaces - depth inside the tree
      int spaces = 0;
      for (size_t i = 0; i < line.size(); ++i)
      {
        if (line[i] == ' ')
          ++spaces;
        else
          break;
      }
      switch (spaces)
      {
      case 0: // this is value for current node
          value->AddCellId(line);;
        break;
      case 1: // country group
      case 2: // country name
      case 3: // region
        value = &tree.AddAtDepth(spaces - 1, CountryTreeNode(line.substr(spaces)));
        break;
      default:
        return false;
      }
    }
    return true;
  }
}
