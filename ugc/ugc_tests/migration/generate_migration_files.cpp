#include "testing/testing.hpp"

#include "ugc/serdes.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"

#include "platform/platform.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/zlib.hpp"

#include "base/file_name_utils.hpp"

#include <array>
#include <string>
#include <unordered_map>

using namespace std;

namespace
{
using Source = ReaderSource<FileReader>;
using MigrationTable = unordered_map<uint32_t, uint32_t>;

array<string, 11> const kVersions{{"171020",
                                   "171117",
                                   "171208",
                                   "180110",
                                   "180126",
                                   "180209",
                                   "180316",
                                   "180417",
                                   "180513",
                                   "180527",
                                   "180528"}};

string const kClassificatorFileName = "classificator.gz";
string const kTypesFileName = "types.gz";
string const kBinFileExtension = ".bin";

string GetUGCDirPath()
{
  return base::JoinPath(GetPlatform().WritableDir(), "ugc_migration");
}

void LoadClassificatorTypesForVersion(string const & version)
{
  auto const folderPath = base::JoinPath("ugc_migration_supported_files", version);
  auto const & p = GetPlatform();

  using Inflate = coding::ZLib::Inflate;

  string classificator;
  {
    auto const r = p.GetReader(base::JoinPath(folderPath, kClassificatorFileName));
    string data;
    r->ReadAsString(data);
    Inflate inflate(Inflate::Format::GZip);
    inflate(data.data(), data.size(), back_inserter(classificator));
  }

  string types;
  {
    auto const r = p.GetReader(base::JoinPath(folderPath, kTypesFileName));
    string data;
    r->ReadAsString(data);
    Inflate inflate(Inflate::Format::GZip);
    inflate(data.data(), data.size(), back_inserter(types));
  }

  classificator::LoadTypes(classificator, types);
}

void LoadTableForVersion(string const & version, MigrationTable & table)
{
  Source source(FileReader(base::JoinPath(GetUGCDirPath(), version + kBinFileExtension)));
  ugc::DeserializerV0<Source> des(source);
  des(table);
}
}  // namespace

UNIT_TEST(UGC_GenerateMigrationFiles)
{
  auto const ugcDirPath = GetUGCDirPath();
  for (auto const & v : kVersions)
  {
    LoadClassificatorTypesForVersion(v);
    auto const & c = classif();

    MigrationTable mapping;
    auto const parse = [&c, &mapping](ClassifObject const *, uint32_t type)
    {
      if (c.IsTypeValid(type))
        mapping.emplace(type, c.GetIndexForType(type));
    };

    c.ForEachTree(parse);

    auto const fileName = v + kBinFileExtension;
    auto const filePath = base::JoinPath(ugcDirPath, fileName);
    {
      FileWriter sink(filePath, FileWriter::Op::OP_WRITE_TRUNCATE);
      ugc::Serializer<FileWriter> ser(sink);
      ser(mapping);
    }

    MigrationTable res;
    LoadTableForVersion(v, res);
    TEST_EQUAL(res, mapping, ());
  }
}

UNIT_TEST(UGC_ValidateMigrationTables)
{
  auto const ugcDirPath = GetUGCDirPath();
  for (auto const & v : kVersions)
  {
    MigrationTable table;
    LoadTableForVersion(v, table);
    TEST(!table.empty(), ("Version", v));

    LoadClassificatorTypesForVersion(v);
    auto const & c = classif();
    auto const parse = [&c, &table](ClassifObject const *, uint32_t type)
    {
      if (c.IsTypeValid(type))
        TEST_EQUAL(table[type], c.GetIndexForType(type), ());
    };

    c.ForEachTree(parse);
  }
}

UNIT_TEST(UGC_TypesFromDifferentVersions)
{
  auto constexpr size = kVersions.size();
  for (size_t i = 0; i < size; ++i)
  {
    auto const & v = kVersions[i];
    MigrationTable table;
    LoadTableForVersion(v, table);
    LoadClassificatorTypesForVersion(v);
    unordered_map<string, uint32_t> namesToTypes;
    {
      auto const & c = classif();
      auto const parse = [&c, &namesToTypes](ClassifObject const *, uint32_t type)
      {
        if (c.IsTypeValid(type))
          namesToTypes.emplace(c.GetReadableObjectName(type), type);
      };

      c.ForEachTree(parse);
    }

    for (size_t j = 0; j < size; ++j)
    {
      if (i == j)
        continue;

      auto const & otherV = kVersions[j];
      MigrationTable otherTable;
      LoadTableForVersion(otherV, otherTable);
      LoadClassificatorTypesForVersion(otherV);
      auto const & c = classif();
      auto const parse = [&c, &namesToTypes, &otherTable, &table](ClassifObject const *, uint32_t otherType) {
        if (c.IsTypeValid(otherType))
        {
          auto const otherReadableName = c.GetReadableObjectName(otherType);
          if (namesToTypes.find(otherReadableName) != namesToTypes.end())
          {
            auto const type = namesToTypes[otherReadableName];
            TEST_EQUAL(otherTable[otherType], table[type], ());
          }
        }
      };
      c.ForEachTree(parse);
    }
  }
}
