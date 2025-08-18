#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class FilesContainerR;
class FilesContainerW;
class FilesMappingContainer;
class Writer;

namespace platform
{
class LocalCountryFile;
}

namespace search
{
// A wrapper class around serialized as an mwm-section rank table.
//
// *NOTE* This wrapper is abstract enough so feel free to change it,
// note that there should always be backward-compatibility. Thus, when
// adding new versions, never change old data format of old versions.

// All rank tables are serialized in the following format:
//
// File offset (bytes)  Field name  Field size (bytes)
// 0                    version     1
// 1                    flags       1
// 8                    data        *
//
// Flags bits:
// 0      - endianess of the stored table, 1 if BigEndian, 0 otherwise.
// [1, 8) - currently not used.

// Data size and contents depend on the version, but note that data
// should always be 8-bytes aligned. Therefore, there is 6-bytes empty
// area between flags and data. Feel free to use it if you need it.
class RankTable
{
public:
  enum Version
  {
    V0 = 0,
    VERSION_COUNT
  };

  virtual ~RankTable() = default;

  static uint8_t constexpr kNoRank = 0;
  /// @return rank of the i-th feature, or kNoRank if there is no rank.
  virtual uint8_t Get(uint64_t i) const = 0;

  // Returns total number of ranks (or features, as there is a 1-1 correspondence).
  virtual uint64_t Size() const = 0;

  // Returns underlying data format version.
  virtual Version GetVersion() const = 0;

  // Serializes rank table.
  virtual void Serialize(Writer & writer) = 0;

  // Copies whole section corresponding to a rank table and
  // deserializes it. Returns nullptr if there're no ranks section or
  // rank table's header is damaged.
  //
  // *NOTE* Return value can outlive |rcont|. Also note that there is
  // undefined behaviour if ranks section exists but internally
  // damaged.
  static std::unique_ptr<RankTable> Load(FilesContainerR const & rcont, std::string const & sectionName);

  // Maps whole section corresponding to a rank table and deserializes
  // it. Returns nullptr if there're no ranks section, rank table's
  // header is damaged or serialized rank table has improper
  // endianness.
  //
  // *NOTE* Return value can't outlive |mcont|, i.e. it must be
  // destructed before |mcont| is closed. Also note that there're
  // undefined behaviour if ranks section exists but internally
  // damaged.
  static std::unique_ptr<RankTable> Load(FilesMappingContainer const & mcont, std::string const & sectionName);
};

// A builder class for rank tables.
class RankTableBuilder
{
public:
  // Force creation of a rank table from array of ranks. Existing rank
  // table is removed (if any). Note that |wcont| must be instantiated
  // as FileWriter::OP_WRITE_EXISTING.
  static void Create(std::vector<uint8_t> const & ranks, FilesContainerW & wcont, std::string const & sectionName);
};

class SearchRankTableBuilder
{
public:
  // Calculates search ranks for all features in an mwm.
  static void CalcSearchRanks(FilesContainerR & rcont, std::vector<uint8_t> & ranks);

  // Following methods create rank table for an mwm.
  // * When rank table already exists and has proper endianness, does nothing.
  // * When rank table already exists but has improper endianness, re-creates it by
  //   reverse mapping.
  // * When rank table does not exist or exists but is damaged, calculates all
  //   features' ranks and creates rank table.
  //
  // Return true if rank table was successfully generated and written
  // or already exists and has correct format.
  static bool CreateIfNotExists(std::string const & mapPath) noexcept;
};
}  // namespace search
