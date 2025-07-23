#pragma once

#include "indexer/complex/serdes_utils.hpp"
#include "indexer/complex/tree_node.hpp"

#include "coding/reader.hpp"
#include "coding/varint.hpp"
#include "coding/writer.hpp"

#include "base/logging.hpp"
#include "base/stl_helpers.hpp"

#include <string>
#include <vector>

namespace complex
{
class ComplexSerdes
{
public:
  using Ids = std::vector<uint32_t>;

  enum class Version : uint8_t
  {
    // There aren't optimized serialization and deserialization. It's experimental verison.
    // todo(m.andrianov): Explore better ways for serialization and deserialization.
    V0,
  };

  static Version const kLatestVersion;

  template <typename Sink>
  static void Serialize(Sink & sink, tree_node::Forest<Ids> const & forest)
  {
    SerializeLatestVersion(sink);
    Serialize(sink, kLatestVersion, forest);
  }

  template <typename Reader>
  static void Deserialize(Reader & reader, tree_node::Forest<Ids> & forest)
  {
    ReaderSource<decltype(reader)> src(reader);
    auto const version = DeserializeVersion(src);
    Deserialize(src, version, forest);
  }

  template <typename Sink>
  static void Serialize(Sink & sink, Version version, tree_node::Forest<Ids> const & forest)
  {
    switch (version)
    {
    case Version::V0: V0::Serialize(sink, forest); break;
    default: UNREACHABLE();
    }
  }

  template <typename Src>
  static void Deserialize(Src & src, Version version, tree_node::Forest<Ids> & forest)
  {
    switch (version)
    {
    case Version::V0: V0::Deserialize(src, forest); break;
    default: UNREACHABLE();
    }
  }

private:
  class V0
  {
  public:
    template <typename Sink>
    static void Serialize(Sink & sink, tree_node::Forest<Ids> const & forest)
    {
      forest.ForEachTree([&](auto const & tree) { Serialize(sink, tree); });
    }

    template <typename Src>
    static void Deserialize(Src & src, tree_node::Forest<Ids> & forest)
    {
      while (src.Size() > 0)
      {
        tree_node::types::Ptr<Ids> tree;
        Deserialize(src, tree);
        forest.Append(tree);
      }
    }

  private:
    template <typename Sink>
    static void Serialize(Sink & sink, tree_node::types::Ptr<Ids> const & tree)
    {
      tree_node::PreOrderVisit(tree, [&](auto const & node)
      {
        coding_utils::WriteCollectionPrimitive(sink, node->GetData());
        auto const size = base::checked_cast<coding_utils::CollectionSizeType>(node->GetChildren().size());
        WriteVarUint(sink, size);
      });
    }

    template <typename Src>
    static void Deserialize(Src & src, tree_node::types::Ptr<Ids> & tree)
    {
      std::function<void(tree_node::types::Ptr<Ids> &)> deserializeTree;
      deserializeTree = [&](auto & tree)
      {
        Ids ids;
        coding_utils::ReadCollectionPrimitive(src, std::back_inserter(ids));
        tree = tree_node::MakeTreeNode(std::move(ids));
        auto const size = ReadVarUint<coding_utils::CollectionSizeType>(src);
        tree_node::types::Ptrs<Ids> children(size);
        for (auto & n : children)
        {
          deserializeTree(n);
          n->SetParent(tree);
        }
        tree->SetChildren(std::move(children));
      };
      deserializeTree(tree);
    }
  };

  template <typename Sink>
  static void SerializeLatestVersion(Sink & sink)
  {
    WriteToSink(sink, base::Underlying(kLatestVersion));
  }

  template <typename Src>
  static Version DeserializeVersion(Src & src)
  {
    return static_cast<Version>(ReadPrimitiveFromSource<std::underlying_type_t<Version>>(src));
  }
};
}  // namespace complex
