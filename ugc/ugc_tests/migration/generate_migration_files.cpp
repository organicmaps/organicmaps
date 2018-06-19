#include "testing/testing.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"

#include "platform/platform.hpp"

#include "ugc/serdes.hpp"

#include <array>
#include <string>
#include <unordered_map>

using namespace std;

namespace
{
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

string const kUGCMigrationDirName = "ugc_migration";
string const kClassificatorFileName = "classificator.txt";
string const kTypesFileName = "types.txt";
string const kBinFileExtension = ".bin";
}  // namespace

UNIT_TEST(UGC_GenerateMigrationFiles)
{
  auto & p = GetPlatform();
  auto const ugcDirPath = my::JoinPath(p.WritableDir(), kUGCMigrationDirName);
  for (auto const & v : kVersions)
  {
    auto const folderPath = my::JoinPath(ugcDirPath, v);
    string classificator;
    {
      auto const r = p.GetReader(my::JoinPath(folderPath, kClassificatorFileName));
      r->ReadAsString(classificator);
    }

    string types;
    {
      auto const r = p.GetReader(my::JoinPath(folderPath, kTypesFileName));
      r->ReadAsString(types);
    }

    classificator::LoadTypes(classificator, types);
    Classificator const & c = classif();

    unordered_map<uint32_t, uint32_t> mapping;
    auto const parse = [&c, &mapping](ClassifObject const * obj, uint32_t type)
    {
      if (c.IsTypeValid(type))
        mapping.emplace(type, c.GetIndexForType(type));
    };

    c.ForEachTree(parse);

    auto const fileName = v + kBinFileExtension;
    auto const filePath = my::JoinPath(ugcDirPath, fileName);
    {
      FileWriter sink(filePath, FileWriter::Op::OP_WRITE_TRUNCATE);
      ugc::Serializer<FileWriter> ser(sink);
      ser(mapping);
    }

    unordered_map<uint32_t, uint32_t> res;
    {
      ReaderSource<FileReader> source(FileReader(filePath, true /* withExceptions */));
      ugc::DeserializerV0<ReaderSource<FileReader>> des(source);
      des(res);
    }

    TEST_EQUAL(res, mapping, ());
  }
}
