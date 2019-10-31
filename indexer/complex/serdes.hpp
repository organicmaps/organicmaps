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

#include <boost/optional.hpp>

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
  static bool Deserialize(Reader & reader, tree_node::Forest<Ids> & forest)
  {
    ReaderSource<decltype(reader)> src(reader);
    auto const version = DeserializeVersion(src);
    return version ? Deserialize(src, *version, forest) : false;
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
  static bool Deserialize(Src & src, Version version, tree_node::Forest<Ids> & forest)
  {
    switch (version)
    {
    case Version::V0: return V0::Deserialize(src, forest);
    default: UNREACHABLE();
    }
  }

private:
  using ByteVector = std::vector<uint8_t>;

  class V0
  {
  public:
    template <typename Sink>
    static void Serialize(Sink & sink, tree_node::Forest<Ids> const & forest)
    {
      ByteVector forestBuffer;
      MemWriter<decltype(forestBuffer)> forestWriter(forestBuffer);
      WriterSink<decltype(forestWriter)> forestMemSink(forestWriter);
      forest.ForEachTree([&](auto const & tree) { Serialize(forestMemSink, tree); });
      coding_utils::WriteCollectionPrimitive(sink, forestBuffer);
    }

    template <typename Src>
    static bool Deserialize(Src & src, tree_node::Forest<Ids> & forest)
    {
      ByteVector forestBuffer;
      coding_utils::ReadCollectionPrimitive(src, std::back_inserter(forestBuffer));
      MemReader forestReader(forestBuffer.data(), forestBuffer.size());
      ReaderSource<decltype(forestReader)> forestSrc(forestReader);
      while (forestSrc.Size() > 0)
      {
        tree_node::types::Ptr<Ids> tree;
        if (!Deserialize(forestSrc, tree))
          return false;

        forest.Append(tree);
      }
      return true;
    }

  private:
    template <typename Sink>
    static void Serialize(Sink & sink, tree_node::types::Ptr<Ids> const & tree)
    {
      uint32_t const base = tree_node::Min(tree, [](auto const & data) {
        auto const it = std::min_element(std::cbegin(data), std::cend(data));
        CHECK(it != std::cend(data), ());
        return *it;
      });
      WriteVarUint(sink, base);
      tree_node::PreOrderVisit(tree, [&](auto const & node) {
        coding_utils::DeltaEncode<uint32_t>(sink, node->GetData(), base);
        auto const size = base::checked_cast<coding_utils::CollectionSizeType>(node->GetChildren().size());
        WriteVarUint(sink, size);
      });
    }

    template <typename Src>
    static bool Deserialize(Src & src, tree_node::types::Ptr<Ids> & tree)
    {
      auto const base = ReadVarUint<uint32_t>(src);
      std::function<bool(tree_node::types::Ptr<Ids> &)> deserializeTree;
      deserializeTree = [&](auto & tree) {
        if (src.Size() == 0)
          return true;

        Ids ids;
        coding_utils::DeltaDecode<uint32_t>(src, std::back_inserter(ids), base);
        tree = tree_node::MakeTreeNode(std::move(ids));
        auto const size = ReadVarUint<coding_utils::CollectionSizeType>(src);
        tree_node::types::Ptrs<Ids> children(size);
        for (auto & n : children)
        {
          if (!deserializeTree(n))
            return false;

          n->SetParent(tree);
        }
        tree->SetChildren(std::move(children));
        return true;
      };
      return deserializeTree(tree);
    }
  };

  template <typename Sink>
  static void SerializeLatestVersion(Sink & sink)
  {
    WriteToSink(sink, base::Underlying(kLatestVersion));
  }

  template <typename Src>
  static boost::optional<Version> DeserializeVersion(Src & src)
  {
    if (src.Size() < sizeof(kLatestVersion))
    {
      LOG(LERROR, ("Unable to deserialize complexes: wrong header version."));
      return {};
    }
    return static_cast<Version>(ReadPrimitiveFromSource<std::underlying_type<Version>::type>(src));
  }
};
}  // namespace complex
