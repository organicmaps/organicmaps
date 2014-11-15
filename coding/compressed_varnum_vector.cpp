#include <iostream>
#include <string>

#include "arithmetic_codec.hpp"
#include "compressed_varnum_vector.hpp"
#include "reader.hpp"
#include "writer.hpp"

#include "../std/unique_ptr.hpp"
#include "../std/vector.hpp"

using std::vector;

#define COUTLN() { std::cout << __LINE__ << std::endl; }

namespace {
  void VarintEncode(vector<uint8_t> & dst, uint64_t n) {
    if (n == 0) {
      dst.push_back(0);
    } else {
      while (n != 0) {
        uint8_t b = n & 0x7F;
        n >>= 7;
        b |= n == 0 ? 0 : 0x80;
        dst.push_back(b);
      }
    }
  }
  void VarintEncode(Writer & writer, uint64_t n) {
    if (n == 0) {
      writer.Write(&n, 1);
    } else {
      while (n != 0) {
        uint8_t b = n & 0x7F;
        n >>= 7;
        b |= n == 0 ? 0 : 0x80;
        writer.Write(&b, 1);
      }
    }
  }
  uint64_t VarintDecode(void * src, uint64_t & offset) {
    uint64_t n = 0;
    int shift = 0;
    while (1) {
      uint8_t b = *(((uint8_t*)src) + offset);
      ASSERT_LESS_OR_EQUAL(shift, 56, ());
      n |= uint64_t(b & 0x7F) << shift;
      ++offset;
      if ((b & 0x80) == 0) break;
      shift += 7;
    }
    return n;
  }
  uint64_t VarintDecode(Reader & reader, uint64_t & offset) {
    uint64_t n = 0;
    int shift = 0;
    while (1) {
      uint8_t b = 0;
      reader.Read(offset, &b, 1);
      ASSERT_LESS_OR_EQUAL(shift, 56, ());
      n |= uint64_t(b & 0x7F) << shift;
      ++offset;
      if ((b & 0x80) == 0) break;
      shift += 7;
    }
    return n;
  }
  uint64_t VarintDecodeReverse(Reader & reader, uint64_t & offset) {
    u8 b = 0;
    do {
      --offset;
      reader.Read(offset, &b, 1);
    } while ((b & 0x80) != 0);
    ++offset;
    u64 begin_offset = offset;
    u64 num = VarintDecode(reader, offset);
    offset = begin_offset;
    return num;
  }

  inline uint32_t NumUsedBits(uint64_t n) {
    uint32_t result = 0;
    while (n != 0) { ++result; n >>= 1; }
    return result;
  }
  vector<uint32_t> SerialFreqsToDistrTable(Reader & reader, uint64_t & decode_offset, uint64_t cnt) {
    vector<uint32_t> freqs;
    for (uint64_t i = 0; i < cnt; ++i) freqs.push_back(VarintDecode(reader, decode_offset));
    return FreqsToDistrTable(freqs);
  }
  
  u64 Max(u64 a, u64 b) { return a > b ? a : b; }
  u64 Min(u64 a, u64 b) { return a < b ? a : b; }
  
  std::string HexEncode(void const * data, u64 size) {
    static char const kHexTable[] = "0123456789ABCDEF";
    std::string hex_str;

    for (size_t i = 0; i < size; ++i) {
      u8 b = *(((u8*)data) + i);
      hex_str.append(1, kHexTable[(b >> 4) & 0xF]);
      hex_str.append(1, kHexTable[b & 0xF]);
    }

    return hex_str;
  }
}

class BitWriter {
public:
  BitWriter(Writer & _writer)
    : writer_(_writer), last_byte_(0), size_(0) {}
  ~BitWriter() { if (size_ % 8 > 0) writer_.Write(&last_byte_, 1); }
  uint64_t NumBitsWritten() const { return size_; }
  void Write(uint64_t bits, uint32_t write_size) {
    if (write_size == 0) return;
    total_bits_ += write_size;
    uint32_t rem_size = size_ % 8;
    ASSERT_LESS_OR_EQUAL(write_size, 64 - rem_size, ());
    if (rem_size > 0) {
      bits <<= rem_size;
      bits |= last_byte_;
      write_size += rem_size;
      size_ -= rem_size;
    }
    uint32_t write_bytes_size = write_size / 8;
    writer_.Write(&bits, write_bytes_size);
    last_byte_ = (bits >> (write_bytes_size * 8)) & ((1 << (write_size % 8)) - 1);
    size_ += write_size;
  }
private:
  Writer & writer_;
  uint8_t last_byte_;
  uint64_t size_;
  uint64_t total_bits_;
};

class BitReader {
public:
  BitReader(Reader & reader)
    : reader_(reader), serial_cur_(0), serial_end_(reader.Size()),
      bits_(0), bits_size_(0), total_bits_read_(0) {}
  uint64_t NumBitsRead() const { return total_bits_read_; }
  uint64_t Read(uint32_t read_size) {
    total_bits_read_ += read_size;
    if (read_size == 0) return 0;
    ASSERT_LESS_OR_EQUAL(read_size, 64, ());
    // First read, sets bits that are in the bits_ buffer.
    uint32_t first_read_size = read_size <= bits_size_ ? read_size : bits_size_;
    uint64_t result = bits_ & (~uint64_t(0) >> (64 - first_read_size));
    bits_ >>= first_read_size;
    bits_size_ -= first_read_size;
    read_size -= first_read_size;
    // Second read, does an extra read using reader_.
    if (read_size > 0) {
      uint32_t read_byte_size = serial_cur_ + sizeof(bits_) <= serial_end_ ? sizeof(bits_) : serial_end_ - serial_cur_;
      reader_.Read(serial_cur_, &bits_, read_byte_size);
      serial_cur_ += read_byte_size;
      bits_size_ += read_byte_size * 8;
      if (read_size > bits_size_) ASSERT_LESS_OR_EQUAL(read_size, bits_size_, ());
      result |= (bits_ & (~uint64_t(0) >> (64 - read_size))) << first_read_size;
      bits_ >>= read_size;
      bits_size_ -= read_size;
      read_size = 0;
    }
    return result;
  }
private:
  Reader & reader_;
  uint64_t serial_cur_;
  uint64_t serial_end_;
  uint64_t bits_;
  uint32_t bits_size_;
  uint64_t total_bits_read_;
};

void BuildCompressedVarnumVector(Writer & writer, NumsSourceFuncT nums_source, u64 nums_cnt, bool support_sums) {
  // Encode header.
  VarintEncode(writer, nums_cnt);
  VarintEncode(writer, c_num_elem_per_table_entry);
  VarintEncode(writer, support_sums ? 1 : 0);

  // Compute frequencies of bits sizes of all nums.
  vector<u32> sizes_freqs(65, 0);
  int32_t max_bits_size = -1;
  for (u64 i = 0; i < nums_cnt; ++i) {
    u64 num = nums_source(i);
    u32 bits_used = NumUsedBits(num);
    ++sizes_freqs[bits_used];
    if (int32_t(bits_used) > max_bits_size) max_bits_size = bits_used;
  }
  sizes_freqs.resize(max_bits_size + 1);
  VarintEncode(writer, sizes_freqs.size());
  for (u32 i = 0; i < sizes_freqs.size(); ++i) VarintEncode(writer, sizes_freqs[i]);
  
  vector<u32> distr_table = FreqsToDistrTable(sizes_freqs);

  vector<u8> encoded_table;
  u64 table_size = nums_cnt == 0 ? 1 : ((nums_cnt - 1) / c_num_elem_per_table_entry) + 2;
  u64 inum = 0, prev_chunk_pos = 0, encoded_nums_size = 0, prev_chunk_sum = 0, sum = 0;
  {
    // Encode starting table entry.
    VarintEncode(encoded_table, 0);
    if (support_sums) VarintEncode(encoded_table, 0);
  }
  for (u64 itable = 0; itable < table_size && inum < nums_cnt; ++itable) {
    // Encode chunk of nums (one chunk for one table entry).    
    vector<u8> encoded_chunk, encoded_bits;
    ArithmeticEncoder arith_enc_sizes(distr_table);
    {
      MemWriter< vector<u8> > encoded_bits_writer(encoded_bits);
      BitWriter bits_writer(encoded_bits_writer);
      for (u64 ichunk_num = 0; ichunk_num < c_num_elem_per_table_entry && inum < nums_cnt; ++ichunk_num, ++inum) {
        u64 num = nums_source(inum);
        u32 bits_used = NumUsedBits(num);
        arith_enc_sizes.Encode(bits_used);
        if (bits_used > 1) bits_writer.Write(num, bits_used - 1);
        sum += num;
        //std::cout << "B(" << bits_used << ", " << num << ")";
      }
    }
    vector<u8> encoded_chunk_sizes = arith_enc_sizes.Finalize();
    //std::cout << "encoded_chunk_sizes.size() == " << encoded_chunk_sizes.size() << std::endl;
    //std::cout << "encoded_bits.size() == " << encoded_bits.size() << std::endl;
    VarintEncode(encoded_chunk, encoded_chunk_sizes.size());
    encoded_chunk.insert(encoded_chunk.end(), encoded_chunk_sizes.begin(), encoded_chunk_sizes.end());
    encoded_chunk.insert(encoded_chunk.end(), encoded_bits.begin(), encoded_bits.end());
    writer.Write(encoded_chunk.data(), encoded_chunk.size());
    encoded_nums_size += encoded_chunk.size();
    //std::cout << u32(encoded_chunk[0]) << u32(encoded_chunk[1]) << u32(encoded_chunk[2]) << u32(encoded_chunk[3]) << std::endl;
    //std::cout << "[" << HexEncode(encoded_chunk_sizes.data(), encoded_chunk_sizes.size()) << "]" << std::endl;
    //std::cout << "[" << HexEncode(encoded_chunk.data(), encoded_chunk.size()) << "]" << std::endl;

    // Encode table entry.
    VarintEncode(encoded_table, encoded_nums_size - prev_chunk_pos);
    if (support_sums) VarintEncode(encoded_table, sum - prev_chunk_sum);
    prev_chunk_pos = encoded_nums_size;
    prev_chunk_sum = sum;
  }
  writer.Write(encoded_table.data(), encoded_table.size());
  VarintEncode(writer, encoded_table.size());
}

struct CompressedVarnumVectorReader::DecodeContext {
  std::unique_ptr<Reader> sizes_arith_dec_reader_;
  std::unique_ptr<ArithmeticDecoder> sizes_arith_dec_;
  std::unique_ptr<Reader> nums_bits_reader_reader_;
  std::unique_ptr<BitReader> nums_bits_reader_;
  u64 nums_left_in_chunk_;
};

CompressedVarnumVectorReader::CompressedVarnumVectorReader(Reader & reader)
  : reader_(reader), nums_cnt_(0), num_elem_per_table_entry_(0), support_sums_(false),
    nums_encoded_offset_(0), decode_ctx_(0) {
  ASSERT_GREATER(reader.Size(), 0, ());
  // Decode header.
  u64 offset = 0;
  nums_cnt_ = VarintDecode(reader_, offset);
  num_elem_per_table_entry_ = VarintDecode(reader_, offset);
  support_sums_ = VarintDecode(reader_, offset) != 0;
  vector<u32> sizes_freqs;
  u64 freqs_cnt = VarintDecode(reader_, offset);
  for (u32 i = 0; i < freqs_cnt; ++i) sizes_freqs.push_back(VarintDecode(reader_, offset));
  distr_table_ = FreqsToDistrTable(sizes_freqs);
  nums_encoded_offset_ = offset;

  // Decode jump table.  
  u64 table_size = nums_cnt_ == 0 ? 0 : ((nums_cnt_ - 1) / num_elem_per_table_entry_) + 1;
  u64 table_decode_offset = reader.Size() - 1;
  u64 table_size_encoded_size = VarintDecodeReverse(reader, table_decode_offset);
  u64 table_encoded_begin = table_decode_offset - table_size_encoded_size;
  u64 table_encoded_end = table_decode_offset;
  u64 prev_pos = 0, prev_sum = 0;
  for (u64 table_offset = table_encoded_begin; table_offset < table_encoded_end;) {
    u64 pos_diff = VarintDecode(reader, table_offset);
    table_pos_.push_back(prev_pos + pos_diff);
    prev_pos += pos_diff;
    if (support_sums_) {
      u64 sum_diff = VarintDecode(reader, table_offset);
      table_sum_.push_back(prev_sum + sum_diff);
      prev_sum += sum_diff;
    }
  }
}

CompressedVarnumVectorReader::~CompressedVarnumVectorReader() {
  if (decode_ctx_) delete decode_ctx_;
}

void CompressedVarnumVectorReader::SetDecodeContext(u64 table_entry_index) {
  ASSERT_LESS(table_entry_index, table_pos_.size() - 1, ());
  u64 decode_offset = nums_encoded_offset_ + table_pos_[table_entry_index];
  u64 encoded_sizes_size = VarintDecode(reader_, decode_offset);
  // Create decode context.
  if (decode_ctx_) delete decode_ctx_;
  decode_ctx_ = new DecodeContext;
  decode_ctx_->sizes_arith_dec_reader_.reset(reader_.CreateSubReader(decode_offset, encoded_sizes_size));
  decode_ctx_->sizes_arith_dec_.reset(new ArithmeticDecoder(*decode_ctx_->sizes_arith_dec_reader_, distr_table_));
  decode_ctx_->nums_bits_reader_reader_.reset(reader_.CreateSubReader(decode_offset + encoded_sizes_size, nums_encoded_offset_ + table_pos_[table_entry_index + 1] - decode_offset - encoded_sizes_size));
  decode_ctx_->nums_bits_reader_.reset(new BitReader(*decode_ctx_->nums_bits_reader_reader_));
  decode_ctx_->nums_left_in_chunk_ = Min((table_entry_index + 1) * num_elem_per_table_entry_, nums_cnt_) - table_entry_index * num_elem_per_table_entry_;
}

void CompressedVarnumVectorReader::FindByIndex(u64 index, u64 & sum_before) {
  ASSERT_LESS(index, nums_cnt_, ());
  u64 table_entry_index = index / num_elem_per_table_entry_;
  u64 index_within_range = index % num_elem_per_table_entry_;
  
  this->SetDecodeContext(table_entry_index);
  
  u64 sum = 0;
  if (support_sums_) sum = table_sum_[table_entry_index];
  for (u64 i = 0; i < index_within_range; ++i) {
    u64 num = this->Read();
    if (support_sums_) sum += num;
  }
  if (support_sums_) sum_before = sum;
}

u64 CompressedVarnumVectorReader::FindBySum(u64 sum, u64 & sum_incl, u64 & cnt_incl) {
  ASSERT(support_sums_, ());
  // First do binary search over select table to find the biggest
  // sum that is less than our.
  u64 l = 0, r = table_pos_.size();
  while (r - l > 1) {
    u64 m = (l + r) / 2;
    if (sum > table_sum_[m]) {
      l = m;
    } else {
      r = m;
    }
  }
  u64 table_entry_index = l;
  cnt_incl = table_entry_index * num_elem_per_table_entry_;
  
  this->SetDecodeContext(table_entry_index);

  sum_incl = table_sum_[table_entry_index];
  u64 num = 0;
  while (sum_incl < sum && cnt_incl < nums_cnt_) {
    num = this->Read();
    sum_incl += num;
    ++cnt_incl;
    if (sum_incl >= sum) break;
  }
  return num;
}

u64 CompressedVarnumVectorReader::Read() {
  ASSERT(decode_ctx_ != 0, ());
  ASSERT_GREATER(decode_ctx_->nums_left_in_chunk_, 0, ());
  u32 bits_used = decode_ctx_->sizes_arith_dec_->Decode();
  if (bits_used == 0) return 0;
  u64 num = (u64(1) << (bits_used - 1)) | decode_ctx_->nums_bits_reader_->Read(bits_used - 1);
  --decode_ctx_->nums_left_in_chunk_;
  return num;
}
