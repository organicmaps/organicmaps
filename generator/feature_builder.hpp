#pragma once

#include "indexer/feature_data.hpp"

#include "platform/platform.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"

#include "base/geo_object_id.hpp"
#include "base/stl_helpers.hpp"

#include <functional>
#include <list>
#include <string>
#include <vector>

namespace serial
{
class GeometryCodingParams;
}  // namespace serial

namespace feature
{
class FeatureBuilder
{
public:
  using PointSeq = std::vector<m2::PointD>;
  using Geometry = std::list<PointSeq>;
  using Buffer = std::vector<char>;
  using Offsets = std::vector<uint32_t>;

  struct SupportingData
  {
    Offsets m_ptsOffset;
    Offsets m_trgOffset;
    uint8_t m_ptsMask = 0;
    uint8_t m_trgMask = 0;
    uint32_t m_ptsSimpMask = 0;
    PointSeq m_innerPts;
    PointSeq m_innerTrg;
    Buffer m_buffer;
  };

  FeatureBuilder();
  // Checks for equality. The error of coordinates is allowed.
  bool operator==(FeatureBuilder const & fb) const;
  // Checks for equality. The error of coordinates isn't allowed. Binary equality check of
  // coordinates is used.
  bool IsExactEq(FeatureBuilder const & fb) const;

  bool IsValid() const;

  /// @name To work with geometry.
  ///@{
  void AssignPoints(PointSeq points);
  void AssignArea(PointSeq && outline, Geometry const & holes);
  void AddPolygon(PointSeq && poly);
  void ResetGeometry();

  m2::RectD const & GetLimitRect() const { return m_limitRect; }
  Geometry const & GetGeometry() const { return m_polygons; }
  PointSeq const & GetOuterGeometry() const
  {
    CHECK(!m_polygons.empty(), ());
    return m_polygons.front();
  }
  GeomType GetGeomType() const { return m_params.GetGeomType(); }
  bool IsGeometryClosed() const;

  static m2::PointD GetGeometryCenter(PointSeq const & pts);
  m2::PointD GetGeometryCenter() const { return GetGeometryCenter(GetOuterGeometry()); }
  m2::PointD GetKeyPoint() const;
  size_t GetPointsCount() const;
  size_t GetPolygonsCount() const { return m_polygons.size(); }
  ///@}

  template <class ToDo>
  void ForEachPoint(ToDo && toDo) const
  {
    if (IsPoint())
    {
      toDo(m_center);
    }
    else
    {
      for (auto const & points : m_polygons)
        for (auto const & pt : points)
          toDo(pt);
    }
  }

  template <class ToDo>
  bool ForAnyPoint(ToDo && toDo) const
  {
    if (IsPoint())
      return toDo(m_center);

    for (auto const & points : m_polygons)
      if (base::AnyOf(points, std::forward<ToDo>(toDo)))
        return true;

    return false;
  }

  template <class ToDo>
  void ForEachPolygon(ToDo && toDo) const
  {
    for (auto const & points : m_polygons)
      toDo(points);
  }

  // To work with geometry type.
  void SetCenter(m2::PointD const & p);
  void SetLinear(bool reverseGeometry = false);
  void SetArea() { m_params.SetGeomType(GeomType::Area); }
  bool IsPoint() const { return GetGeomType() == GeomType::Point; }
  bool IsLine() const { return GetGeomType() == GeomType::Line; }
  bool IsArea() const { return GetGeomType() == GeomType::Area; }

  /// @name To work with types.
  ///@{
  void SetType(uint32_t type) { m_params.SetType(type); }
  void AddType(uint32_t type) { m_params.AddType(type); }
  bool PopExactType(uint32_t type) { return m_params.PopExactType(type); }

  template <class Fn>
  bool RemoveTypesIf(Fn && fn)
  {
    base::EraseIf(m_params.m_types, std::forward<Fn>(fn));
    return m_params.m_types.empty();
  }

  bool HasType(uint32_t t) const { return m_params.IsTypeExist(t); }
  bool HasType(uint32_t t, uint8_t level) const { return m_params.IsTypeExist(t, level); }
  FeatureParams::Types const & GetTypes() const { return m_params.m_types; }
  size_t GetTypesCount() const { return m_params.m_types.size(); }
  ///@}

  // Actually, "SetName" is a better name for this function ...
  bool AddName(std::string_view lang, std::string_view name);
  void SetName(int8_t lang, std::string_view name);
  void SetParams(FeatureBuilderParams const & params) { m_params.SetParams(params); }

  FeatureBuilderParams const & GetParams() const { return m_params; }
  FeatureBuilderParams & GetParams() { return m_params; }
  std::string_view GetName(int8_t lang = StringUtf8Multilang::kDefaultCode) const;
  StringUtf8Multilang const & GetMultilangName() const { return m_params.name; }
  uint8_t GetRank() const { return m_params.rank; }

  Metadata const & GetMetadata() const { return m_params.GetMetadata(); }
  Metadata & GetMetadata() { return m_params.GetMetadata(); }

  // To work with types and names based on drawing.
  // Check classificator types for their compatibility with feature geometry type.
  // Need to call when using any classificator types manipulating.
  // Return false If no any valid types.
  bool RemoveInvalidTypes();
  // Clear name if it's not visible in scale range [minS, maxS].
  void RemoveNameIfInvisible(int minS = 0, int maxS = 1000);
  void RemoveUselessNames();
  bool IsDrawableInRange(int lowScale, int highScale) const;

  /// @name Serialization.
  ///@{
  bool PreSerialize();
  bool PreSerializeAndRemoveUselessNamesForIntermediate();

  void SerializeForIntermediate(Buffer & data) const;
  void DeserializeFromIntermediate(Buffer & data);

  // These methods use geometry without loss of accuracy.
  void SerializeAccuratelyForIntermediate(Buffer & data) const;
  void DeserializeAccuratelyFromIntermediate(Buffer & data);

  bool PreSerializeAndRemoveUselessNamesForMwm(SupportingData const & data);
  void SerializeForMwm(SupportingData & data, serial::GeometryCodingParams const & params) const;
  ///@}

  // Get common parameters of feature.
  TypesHolder GetTypesHolder() const;

  /// @name To work with osm ids.
  ///@{
  void AddOsmId(base::GeoObjectId id);
  void SetOsmId(base::GeoObjectId id);
  base::GeoObjectId GetFirstOsmId() const;
  base::GeoObjectId GetLastOsmId() const;
  // Returns an id of the most general element: node's one if there is no area or relation,
  // area's one if there is no relation, and relation id otherwise.
  base::GeoObjectId GetMostGenericOsmId() const;
  bool HasOsmIds() const { return !m_osmIds.empty(); }
  std::string DebugPrintIDs() const;
  ///@}

  // To work with coasts.
  void SetCoastCell(int64_t iCell) { m_coastCell = iCell; }
  bool IsCoastCell() const { return (m_coastCell != -1); }

  friend std::string DebugPrint(FeatureBuilder const & fb);

protected:
  // Can be one of the following:
  // - point in point-feature
  // - origin point of text [future] in line-feature
  // - origin point of text or symbol in area-feature
  m2::PointD m_center;  // Check  HEADER_HAS_POINT
  // List of geometry polygons.
  Geometry m_polygons;  // Check HEADER_IS_AREA
  m2::RectD m_limitRect;
  std::vector<base::GeoObjectId> m_osmIds;
  FeatureBuilderParams m_params;
  /// Not used in GEOM_POINTs
  int64_t m_coastCell;
};

// SerializationPolicy serialization and deserialization.
namespace serialization_policy
{
enum class SerializationVersion : uint32_t
{
  Undefined,
  MinSize,
  MaxAccuracy
};

using TypeSerializationVersion = typename std::underlying_type<SerializationVersion>::type;

struct MinSize
{
  auto static const kSerializationVersion = static_cast<TypeSerializationVersion>(SerializationVersion::MinSize);

  static void Serialize(FeatureBuilder const & fb, FeatureBuilder::Buffer & data) { fb.SerializeForIntermediate(data); }

  static void Deserialize(FeatureBuilder & fb, FeatureBuilder::Buffer & data) { fb.DeserializeFromIntermediate(data); }
};

struct MaxAccuracy
{
  auto static const kSerializationVersion = static_cast<TypeSerializationVersion>(SerializationVersion::MinSize);

  static void Serialize(FeatureBuilder const & fb, FeatureBuilder::Buffer & data)
  {
    fb.SerializeAccuratelyForIntermediate(data);
  }

  static void Deserialize(FeatureBuilder & fb, FeatureBuilder::Buffer & data)
  {
    fb.DeserializeAccuratelyFromIntermediate(data);
  }
};
}  // namespace serialization_policy

// TODO(maksimandrianov): I would like to support the verification of serialization versions,
// but this requires reworking of FeatureCollector class and its derived classes. It is in future
// plans

// template <class SerializationPolicy, class Source>
// void TryReadAndCheckVersion(Source & src)
//{
//  if (src.Size() - src.Pos() >= sizeof(serialization_policy::TypeSerializationVersion))
//  {
//    auto const type = ReadVarUint<serialization_policy::TypeSerializationVersion>(src);
//    CHECK_EQUAL(type, SerializationPolicy::kSerializationVersion, ());
//  }
//  else
//  {
//    LOG(LWARNING, ("Unable to read file version."))
//  }
//}

// Read feature from feature source.
template <class SerializationPolicy = serialization_policy::MaxAccuracy, class Source>
void ReadFromSourceRawFormat(Source & src, FeatureBuilder & fb)
{
  uint32_t const sz = ReadVarUint<uint32_t>(src);
  typename FeatureBuilder::Buffer buffer(sz);
  src.Read(&buffer[0], sz);
  SerializationPolicy::Deserialize(fb, buffer);
}

// Process features in features file.
template <class SerializationPolicy = serialization_policy::MaxAccuracy, class ToDo>
void ForEachFeatureRawFormat(std::string const & filename, ToDo && toDo)
{
  FileReader reader(filename);
  ReaderSource<FileReader> src(reader);
  // TryReadAndCheckVersion<SerializationPolicy>(src);
  auto const fileSize = reader.Size();
  auto currPos = src.Pos();
  // read features one by one
  while (currPos < fileSize)
  {
    FeatureBuilder fb;
    ReadFromSourceRawFormat<SerializationPolicy>(src, fb);
    toDo(std::move(fb), currPos);
    currPos = src.Pos();
  }
}

template <class SerializationPolicy = serialization_policy::MaxAccuracy>
std::vector<FeatureBuilder> ReadAllDatRawFormat(std::string const & fileName)
{
  std::vector<FeatureBuilder> fbs;
  // Happens in tests when World or Country file is empty (no valid Features to emit).
  if (Platform::IsFileExistsByFullPath(fileName))
  {
    ForEachFeatureRawFormat<SerializationPolicy>(
        fileName, [&](FeatureBuilder && fb, uint64_t) { fbs.emplace_back(std::move(fb)); });
  }
  return fbs;
}

template <class SerializationPolicy = serialization_policy::MaxAccuracy, class Writer = FileWriter>
class FeatureBuilderWriter
{
public:
  explicit FeatureBuilderWriter(std::string const & filename, bool mangleName = false,
                                FileWriter::Op op = FileWriter::Op::OP_WRITE_TRUNCATE)
    : m_filename(filename)
    , m_mangleName(mangleName)
    , m_writer(std::make_unique<Writer>(m_mangleName ? m_filename + "_" : m_filename, op))
  {
    // TODO(maksimandrianov): I would like to support the verification of serialization versions,
    // but this requires reworking of FeatureCollector class and its derived classes. It is in
    // future plans WriteVarUint(m_writer,
    // static_cast<serialization_policy::TypeSerializationVersion>(SerializationPolicy::kSerializationVersion));
  }

  explicit FeatureBuilderWriter(std::string const & filename, FileWriter::Op op)
    : FeatureBuilderWriter(filename, false /* mangleName */, op)
  {}

  ~FeatureBuilderWriter()
  {
    if (m_mangleName)
    {
      // Flush and close.
      auto const currentFilename = m_writer->GetName();
      m_writer.reset();
      CHECK(base::RenameFileX(currentFilename, m_filename), (currentFilename, m_filename));
    }
  }

  void Write(FeatureBuilder const & fb) { Write(*m_writer, fb); }

  template <typename Sink>
  static void Write(Sink & writer, FeatureBuilder const & fb)
  {
    FeatureBuilder::Buffer buffer;
    SerializationPolicy::Serialize(fb, buffer);
    WriteVarUint(writer, static_cast<uint32_t>(buffer.size()));
    writer.Write(buffer.data(), buffer.size() * sizeof(FeatureBuilder::Buffer::value_type));
  }

private:
  std::string m_filename;
  bool m_mangleName = false;
  std::unique_ptr<Writer> m_writer;
};
}  // namespace feature
