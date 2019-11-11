#include "testing/testing.hpp"

#include "indexer/complex/serdes.hpp"
#include "indexer/complex/tree_node.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include <cstdint>
#include <vector>

namespace
{
using ByteVector = std::vector<uint8_t>;

decltype(auto) GetForest()
{
  auto tree1 = tree_node::MakeTreeNode<complex::ComplexSerdes::Ids>({1, 2, 3});
  auto node11 = tree_node::MakeTreeNode<complex::ComplexSerdes::Ids>({11, 12, 13});
  tree_node::Link(tree_node::MakeTreeNode<complex::ComplexSerdes::Ids>({111}), node11);
  tree_node::Link(tree_node::MakeTreeNode<complex::ComplexSerdes::Ids>({112, 113}), node11);
  tree_node::Link(tree_node::MakeTreeNode<complex::ComplexSerdes::Ids>({114}), node11);
  tree_node::Link(node11, tree1);
  tree_node::Link(tree_node::MakeTreeNode<complex::ComplexSerdes::Ids>({6, 7}), tree1);

  auto tree2 = tree_node::MakeTreeNode<complex::ComplexSerdes::Ids>({9});
  tree_node::Link(tree_node::MakeTreeNode<complex::ComplexSerdes::Ids>({21, 22, 23}), tree2);
  tree_node::Link(tree_node::MakeTreeNode<complex::ComplexSerdes::Ids>({24, 25}), tree2);

  tree_node::Forest<complex::ComplexSerdes::Ids> forest;
  forest.Append(tree1);
  forest.Append(tree2);
  return forest;
}

UNIT_TEST(Complex_SerdesV0)
{
  auto const expectedForest = GetForest();
  ByteVector buffer;
  {
    MemWriter<decltype(buffer)> writer(buffer);
    WriterSink<decltype(writer)> sink(writer);
    complex::ComplexSerdes::Serialize(sink, complex::ComplexSerdes::Version::V0, expectedForest);
  }
  {
    MemReader reader(buffer.data(), buffer.size());
    ReaderSource<decltype(reader)> src(reader);
    tree_node::Forest<complex::ComplexSerdes::Ids> forest;
    complex::ComplexSerdes::Deserialize(src, complex::ComplexSerdes::Version::V0, forest);
    TEST_EQUAL(forest, expectedForest, ());
  }
}

UNIT_TEST(Complex_Serdes)
{
  auto const expectedForest = GetForest();
  ByteVector buffer;
  {
    MemWriter<decltype(buffer)> writer(buffer);
    WriterSink<decltype(writer)> sink(writer);
    complex::ComplexSerdes::Serialize(sink, expectedForest);
  }
  {
    MemReader reader(buffer.data(), buffer.size());
    tree_node::Forest<complex::ComplexSerdes::Ids> forest;
    complex::ComplexSerdes::Deserialize(reader, forest);
    TEST_EQUAL(forest, expectedForest, ());
  }
}
}  // namespace
