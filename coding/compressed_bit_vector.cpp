#include "compressed_bit_vector.hpp"

#include "arithmetic_codec.hpp"
#include "reader.hpp"
#include "writer.hpp"

#include "../base/assert.hpp"

using std::vector;

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
}

class BitWriter {
public:
  BitWriter(Writer & _writer)
    : writer_(_writer), last_byte_(0), size_(0) {}
  ~BitWriter() { Finalize(); }
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
  void Finalize() { if (size_ % 8 > 0) writer_.Write(&last_byte_, 1); }
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
  uint64_t NumBitsWritten() const { return total_bits_read_; }
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

void BuildCompressedBitVector(Writer & writer, vector<uint32_t> const & pos_ones, int chosen_enc_type) {
  uint32_t const c_block_size = 7;
  // First stage of compression is analysis run through data ones.
  uint64_t num_bytes_diffs_enc_vint = 0, num_bytes_ranges_enc_vint = 0, num_bits_diffs_enc_arith = 0, num_bits_ranges_enc_arith = 0;
  int64_t prev_one_pos = -1;
  uint64_t ones_range_len = 0;
  vector<uint32_t> diffs_sizes_freqs(65, 0), ranges0_sizes_freqs(65, 0), ranges1_sizes_freqs(65, 0);
  for (uint32_t i = 0; i < pos_ones.size(); ++i) {
    ASSERT_LESS(prev_one_pos, pos_ones[i], ());
    // Accumulate size of diff encoding.
    uint64_t diff = pos_ones[i] - prev_one_pos;
    uint32_t diff_bitsize = NumUsedBits(diff - 1);
    num_bytes_diffs_enc_vint += (diff_bitsize + c_block_size - 1) / c_block_size;
    num_bits_diffs_enc_arith += diff_bitsize > 0 ? diff_bitsize - 1 : 0;
    ++diffs_sizes_freqs[diff_bitsize];
    // Accumulate sizes of ranges encoding.
    if (pos_ones[i] - prev_one_pos > 1) {
      if (ones_range_len > 0) {
        // Accumulate size of ones-range encoding.
        uint32_t ones_range_len_bitsize = NumUsedBits(ones_range_len - 1);
        num_bytes_ranges_enc_vint += (ones_range_len_bitsize + c_block_size - 1) / c_block_size;
        num_bits_ranges_enc_arith += ones_range_len_bitsize > 0 ? ones_range_len_bitsize - 1 : 0;
        ++ranges1_sizes_freqs[ones_range_len_bitsize];
        ones_range_len = 0;
      }
      // Accumulate size of zeros-range encoding.
      uint32_t zeros_range_len_bitsize = NumUsedBits(pos_ones[i] - prev_one_pos - 2);
      num_bytes_ranges_enc_vint += (zeros_range_len_bitsize + c_block_size - 1) / c_block_size;
      num_bits_ranges_enc_arith += zeros_range_len_bitsize > 0 ? zeros_range_len_bitsize - 1 : 0;
      ++ranges0_sizes_freqs[zeros_range_len_bitsize];
    }
    ++ones_range_len;
    prev_one_pos = pos_ones[i];
  }
  // Accumulate size of remaining ones-range encoding.
  if (ones_range_len > 0) {
    uint32_t ones_range_len_bitsize = NumUsedBits(ones_range_len - 1);
    num_bytes_ranges_enc_vint += (ones_range_len_bitsize + c_block_size - 1) / c_block_size;
    num_bits_ranges_enc_arith = ones_range_len_bitsize > 0 ? ones_range_len_bitsize - 1 : 0;
    ++ranges1_sizes_freqs[ones_range_len_bitsize];
    ones_range_len = 0;
  }
  // Compute arithmetic encoding size.
  uint64_t diffs_sizes_total_freq = 0, ranges0_sizes_total_freq = 0, ranges1_sizes_total_freq = 0;
  for (uint32_t i = 0; i < diffs_sizes_freqs.size(); ++i) diffs_sizes_total_freq += diffs_sizes_freqs[i];
  for (uint32_t i = 0; i < ranges0_sizes_freqs.size(); ++i) ranges0_sizes_total_freq += ranges0_sizes_freqs[i];
  for (uint32_t i = 0; i < ranges1_sizes_freqs.size(); ++i) ranges1_sizes_total_freq += ranges1_sizes_freqs[i];
  // Compute number of bits for arith encoded diffs sizes.
  double num_sizes_bits_diffs_enc_arith = 0;
  uint32_t nonzero_diffs_sizes_freqs_end = 0;
  for (uint32_t i = 0; i < diffs_sizes_freqs.size(); ++i) {
    if (diffs_sizes_freqs[i] > 0) {
      double prob = double(diffs_sizes_freqs[i]) / diffs_sizes_total_freq;
      num_sizes_bits_diffs_enc_arith += - prob * log(prob) / log(2);
      nonzero_diffs_sizes_freqs_end = i + 1;
    }
  }
  vector<uint8_t> diffs_sizes_freqs_serial;
  for (uint32_t i = 0; i < nonzero_diffs_sizes_freqs_end; ++i) VarintEncode(diffs_sizes_freqs_serial, diffs_sizes_freqs[i]);
  uint64_t num_bytes_diffs_enc_arith = 4 + diffs_sizes_freqs_serial.size() + (uint64_t(num_sizes_bits_diffs_enc_arith * diffs_sizes_total_freq + 0.999) + 7) / 8 + (num_bits_diffs_enc_arith + 7) /8;
  // Compute number of bits for arith encoded ranges sizes.
  double num_sizes_bits_ranges0_enc_arith = 0;
  uint32_t nonzero_ranges0_sizes_freqs_end = 0;
  for (uint32_t i = 0; i < ranges0_sizes_freqs.size(); ++i) {
    if (ranges0_sizes_freqs[i] > 0) {
      double prob = double(ranges0_sizes_freqs[i]) / ranges0_sizes_total_freq;
      num_sizes_bits_ranges0_enc_arith += - prob * log(prob) / log(2);
      nonzero_ranges0_sizes_freqs_end = i + 1;
    }
  }
  double num_sizes_bits_ranges1_enc_arith = 0;
  uint32_t nonzero_ranges1_sizes_freqs_end = 0;
  for (uint32_t i = 0; i < ranges1_sizes_freqs.size(); ++i) {
    if (ranges1_sizes_freqs[i] > 0) {
      double prob = double(ranges1_sizes_freqs[i]) / ranges1_sizes_total_freq;
      num_sizes_bits_ranges1_enc_arith += - prob * log(prob) / log(2);
      nonzero_ranges1_sizes_freqs_end = i + 1;
    }
  }
  vector<uint8_t> ranges0_sizes_freqs_serial, ranges1_sizes_freqs_serial;
  for (uint32_t i = 0; i < nonzero_ranges0_sizes_freqs_end; ++i) VarintEncode(ranges0_sizes_freqs_serial, ranges0_sizes_freqs[i]);
  for (uint32_t i = 0; i < nonzero_ranges1_sizes_freqs_end; ++i) VarintEncode(ranges1_sizes_freqs_serial, ranges1_sizes_freqs[i]);
  uint64_t num_bytes_ranges_enc_arith = 4 + ranges0_sizes_freqs_serial.size() + ranges1_sizes_freqs_serial.size() +
    (uint64_t(num_sizes_bits_ranges0_enc_arith * ranges0_sizes_total_freq + 0.999) + 7) / 8 + (uint64_t(num_sizes_bits_ranges1_enc_arith * ranges1_sizes_total_freq + 0.999) + 7) / 8 +
    (num_bits_ranges_enc_arith + 7) / 8;

  // Find minimum among 4 types of encoding.
  vector<uint64_t> num_bytes_per_enc = {num_bytes_diffs_enc_vint, num_bytes_ranges_enc_vint, num_bytes_diffs_enc_arith, num_bytes_ranges_enc_arith};
  uint32_t enc_type = 0;
  if (chosen_enc_type != -1) { ASSERT(0 <= chosen_enc_type && chosen_enc_type <= 3, ()); enc_type = chosen_enc_type; }
  else if (num_bytes_per_enc[0] <= num_bytes_per_enc[1] && num_bytes_per_enc[0] <= num_bytes_per_enc[2] && num_bytes_per_enc[0] <= num_bytes_per_enc[3]) enc_type = 0;
  else if (num_bytes_per_enc[1] <= num_bytes_per_enc[0] && num_bytes_per_enc[1] <= num_bytes_per_enc[2] && num_bytes_per_enc[1] <= num_bytes_per_enc[3]) enc_type = 1;
  else if (num_bytes_per_enc[2] <= num_bytes_per_enc[0] && num_bytes_per_enc[2] <= num_bytes_per_enc[1] && num_bytes_per_enc[2] <= num_bytes_per_enc[3]) enc_type = 2;
  else if (num_bytes_per_enc[3] <= num_bytes_per_enc[0] && num_bytes_per_enc[3] <= num_bytes_per_enc[1] && num_bytes_per_enc[3] <= num_bytes_per_enc[2]) enc_type = 3;

  if (enc_type == 0) {
    // Diffs-Varint encoding.

    int64_t prev_one_pos = -1;
    bool is_empty = pos_ones.empty();
    // Encode encoding type and first diff.
    if (is_empty) {
      VarintEncode(writer, enc_type + (1 << 2));
    } else {
      VarintEncode(writer, enc_type + (0 << 2) + ((pos_ones[0] - prev_one_pos - 1) << 3));
      prev_one_pos = pos_ones[0];
    }
    for (uint32_t i = 1; i < pos_ones.size(); ++i) {
      ASSERT_GREATER(pos_ones[i], prev_one_pos, ());
      // Encode one's pos (diff - 1).
      VarintEncode(writer, pos_ones[i] - prev_one_pos - 1);
      prev_one_pos = pos_ones[i];
    }
  } else if (enc_type == 2) {
    // Diffs-Arith encoding.
    
    // Encode encoding type plus number of freqs in the table.
    VarintEncode(writer, enc_type + (nonzero_diffs_sizes_freqs_end << 2));
    // Encode freqs table.
    writer.Write(diffs_sizes_freqs_serial.data(), diffs_sizes_freqs_serial.size());
    uint64_t tmp_offset = 0;
    MemReader diffs_sizes_freqs_serial_reader(diffs_sizes_freqs_serial.data(), diffs_sizes_freqs_serial.size());
    vector<uint32_t> distr_table = SerialFreqsToDistrTable(
      diffs_sizes_freqs_serial_reader, tmp_offset, nonzero_diffs_sizes_freqs_end
    );

    {
      // First stage. Encode all bits sizes of all diffs using ArithmeticEncoder.
      ArithmeticEncoder arith_enc(distr_table);
      int64_t prev_one_pos = -1;
      uint64_t cnt_elements = 0;
      for (uint64_t i = 0; i < pos_ones.size(); ++i) {
        ASSERT_GREATER(pos_ones[i], prev_one_pos, ());
        uint32_t bits_used = NumUsedBits(pos_ones[i] - prev_one_pos - 1);
        arith_enc.Encode(bits_used);
        ++cnt_elements;
        prev_one_pos = pos_ones[i];
      }
      vector<uint8_t> serial_sizes_enc = arith_enc.Finalize();
      // Store number of compressed elements.
      VarintEncode(writer, cnt_elements);
      // Store compressed size of encoded sizes.
      VarintEncode(writer, serial_sizes_enc.size());
      // Store serial sizes.
      writer.Write(serial_sizes_enc.data(), serial_sizes_enc.size());
    }
    {
      // Second Stage. Encode all bits of all diffs using BitWriter.
      BitWriter bit_writer(writer);
      int64_t prev_one_pos = -1;
      uint64_t total_read_bits = 0;
      uint64_t total_read_cnts = 0;
      for (uint64_t i = 0; i < pos_ones.size(); ++i) {
        ASSERT_GREATER(pos_ones[i], prev_one_pos, ());
        // Encode one's pos (diff - 1).
        uint64_t diff = pos_ones[i] - prev_one_pos - 1;
        uint32_t bits_used = NumUsedBits(diff);
        if (bits_used > 1) {
          // Most significant bit is always 1 for non-zero diffs, so don't store it.
          --bits_used;
          bit_writer.Write(diff, bits_used);
          total_read_bits += bits_used;
          ++total_read_cnts;
        }
        prev_one_pos = pos_ones[i];
      }
    }
  } else if (enc_type == 1) {
    // Ranges-Varint encoding.
    
    // If bit vector starts with 1.
    bool is_first_one = pos_ones.size() > 0 && pos_ones.front() == 0;
    // Encode encoding type plus flag if first is 1.
    VarintEncode(writer, enc_type + ((is_first_one ? 1 : 0) << 2));
    int64_t prev_one_pos = -1;
    uint64_t ones_range_len = 0;
    for (uint32_t i = 0; i < pos_ones.size(); ++i) {
      ASSERT_GREATER(pos_ones[i], prev_one_pos, ());
      if (pos_ones[i] - prev_one_pos > 1) {
        if (ones_range_len > 0) {
          // Encode ones range size - 1.
          VarintEncode(writer, ones_range_len - 1);
          ones_range_len = 0;
        }
        // Encode zeros range size - 1.
        VarintEncode(writer, pos_ones[i] - prev_one_pos - 2);
      }
      ++ones_range_len;
      prev_one_pos = pos_ones[i];
    }
    if (ones_range_len > 0) {
      // Encode last ones range size.
      VarintEncode(writer, ones_range_len - 1);
      ones_range_len = 0;
    }
  } else if (enc_type == 3) {
    // Ranges-Arith encoding.

    // If bit vector starts with 1.
    bool is_first_one = pos_ones.size() > 0 && pos_ones.front() == 0;
    // Encode encoding type plus flag if first is 1 plus count of sizes freqs.
    VarintEncode(writer, enc_type + ((is_first_one ? 1 : 0) << 2) + (nonzero_ranges0_sizes_freqs_end << 3));
    VarintEncode(writer, nonzero_ranges1_sizes_freqs_end);
    // Encode freqs table.
    writer.Write(ranges0_sizes_freqs_serial.data(), ranges0_sizes_freqs_serial.size());
    writer.Write(ranges1_sizes_freqs_serial.data(), ranges1_sizes_freqs_serial.size());
    // Create distr tables.
    uint64_t tmp_offset = 0;
    MemReader ranges0_sizes_freqs_serial_reader(ranges0_sizes_freqs_serial.data(), ranges0_sizes_freqs_serial.size());
    vector<uint32_t> distr_table0 = SerialFreqsToDistrTable(
      ranges0_sizes_freqs_serial_reader, tmp_offset, nonzero_ranges0_sizes_freqs_end
    );
    tmp_offset = 0;
    MemReader ranges1_sizes_freqs_serial_reader(ranges1_sizes_freqs_serial.data(), ranges1_sizes_freqs_serial.size());
    vector<uint32_t> distr_table1 = SerialFreqsToDistrTable(
      ranges1_sizes_freqs_serial_reader, tmp_offset, nonzero_ranges1_sizes_freqs_end
    );

    {
      // First stage, encode all ranges bits sizes using ArithmeticEncoder.

      // Encode number of compressed elements.
      ArithmeticEncoder arith_enc0(distr_table0), arith_enc1(distr_table1);
      int64_t prev_one_pos = -1;
      uint64_t ones_range_len = 0;
      // Total number of compressed elements (ranges sizes).
      uint64_t cnt_elements0 = 0, cnt_elements1 = 0;
      for (uint32_t i = 0; i < pos_ones.size(); ++i) {
        ASSERT_GREATER(pos_ones[i], prev_one_pos, ());
        if (pos_ones[i] - prev_one_pos > 1) {
          if (ones_range_len > 0) {
            // Encode ones range bits size.
            uint32_t bits_used = NumUsedBits(ones_range_len - 1);
            arith_enc1.Encode(bits_used);
            ++cnt_elements1;
            ones_range_len = 0;
          }
          // Encode zeros range bits size - 1.
          uint32_t bits_used = NumUsedBits(pos_ones[i] - prev_one_pos - 2);
          arith_enc0.Encode(bits_used);
          ++cnt_elements0;
        }
        ++ones_range_len;
        prev_one_pos = pos_ones[i];
      }
      if (ones_range_len > 0) {
        // Encode last ones range size - 1.
        uint32_t bits_used = NumUsedBits(ones_range_len - 1);
        arith_enc1.Encode(bits_used);
        ++cnt_elements1;
        ones_range_len = 0;
      }
      vector<uint8_t> serial0_sizes_enc = arith_enc0.Finalize(), serial1_sizes_enc = arith_enc1.Finalize();
      // Store number of compressed elements.
      VarintEncode(writer, cnt_elements0);
      VarintEncode(writer, cnt_elements1);
      // Store size of encoded bits sizes.
      VarintEncode(writer, serial0_sizes_enc.size());
      VarintEncode(writer, serial1_sizes_enc.size());
      // Store serial sizes.
      writer.Write(serial0_sizes_enc.data(), serial0_sizes_enc.size());
      writer.Write(serial1_sizes_enc.data(), serial1_sizes_enc.size());
    }

    {
      // Second stage, encode all ranges bits using BitWriter.
      BitWriter bit_writer(writer);
      int64_t prev_one_pos = -1;
      uint64_t ones_range_len = 0;
      for (uint32_t i = 0; i < pos_ones.size(); ++i) {
        ASSERT_GREATER(pos_ones[i], prev_one_pos, ());
        if (pos_ones[i] - prev_one_pos > 1) {
          if (ones_range_len > 0) {
            // Encode ones range bits size.
            uint32_t bits_used = NumUsedBits(ones_range_len - 1);
            if (bits_used > 1) {
              // Most significant bit for non-zero values is always 1, don't encode it.
              --bits_used;
              bit_writer.Write(ones_range_len - 1, bits_used);
            }
            ones_range_len = 0;
          }
          // Encode zeros range bits size - 1.
          uint32_t bits_used = NumUsedBits(pos_ones[i] - prev_one_pos - 2);
          if (bits_used > 1) {
            // Most significant bit for non-zero values is always 1, don't encode it.
            --bits_used;
            bit_writer.Write(pos_ones[i] - prev_one_pos - 2, bits_used);
          }
        }
        ++ones_range_len;
        prev_one_pos = pos_ones[i];
      }
      if (ones_range_len > 0) {
        // Encode last ones range size - 1.
        uint32_t bits_used = NumUsedBits(ones_range_len - 1);
        if (bits_used > 1) {
          // Most significant bit for non-zero values is always 1, don't encode it.
          --bits_used;
          bit_writer.Write(ones_range_len - 1, bits_used);
        }
        ones_range_len = 0;
      }
    }
  }
}

vector<uint32_t> DecodeCompressedBitVector(Reader & reader) {
  uint64_t serial_size = reader.Size();
  vector<uint32_t> pos_ones;
  uint64_t decode_offset = 0;
  uint64_t header = VarintDecode(reader, decode_offset);
  uint32_t enc_type = header & 3;
  ASSERT_LESS(enc_type, 4, ());
  if (enc_type == 0) {
    // Diffs-Varint encoded.
    int64_t prev_one_pos = -1;
    // For non-empty vectors first diff is taken from header number.
    bool is_empty = (header & 4) != 0;
    if (!is_empty) {
      pos_ones.push_back(header >> 3);
      prev_one_pos = pos_ones.back();
    }
    while (decode_offset < serial_size) {
      pos_ones.push_back(prev_one_pos + VarintDecode(reader, decode_offset) + 1);
      prev_one_pos = pos_ones.back();
    }
  } else if (enc_type == 2) {
    // Diffs-Arith encoded.
    uint64_t freqs_cnt = header >> 2;
    vector<uint32_t> distr_table = SerialFreqsToDistrTable(reader, decode_offset, freqs_cnt);
    uint64_t cnt_elements = VarintDecode(reader, decode_offset);
    uint64_t enc_sizes_bytesize = VarintDecode(reader, decode_offset);
    vector<uint32_t> bits_used_vec;
    Reader * arith_dec_reader = reader.CreateSubReader(decode_offset, enc_sizes_bytesize);
    ArithmeticDecoder arith_dec(*arith_dec_reader, distr_table);
    for (uint64_t i = 0; i < cnt_elements; ++i) bits_used_vec.push_back(arith_dec.Decode());
    decode_offset += enc_sizes_bytesize;
    Reader * bit_reader_reader = reader.CreateSubReader(decode_offset, serial_size - decode_offset);
    BitReader bit_reader(*bit_reader_reader);
    int64_t prev_one_pos = -1;
    for (uint64_t i = 0; i < cnt_elements; ++i) {
      uint32_t bits_used = bits_used_vec[i];
      uint64_t diff = 0;
      if (bits_used > 0) diff = ((uint64_t(1) << (bits_used - 1)) | bit_reader.Read(bits_used - 1)) + 1; else diff = 1;
      pos_ones.push_back(prev_one_pos + diff);
      prev_one_pos += diff;
    }
    decode_offset = serial_size;
  } else if (enc_type == 1) {
    // Ranges-Varint encoding.
    
    // If bit vector starts with 1.
    bool is_first_one = ((header >> 2) & 1) == 1;
    uint64_t sum = 0;
    while (decode_offset < serial_size) {
      uint64_t zeros_range_size = 0;
      // Don't read zero range size for the first time if first bit is 1.
      if (!is_first_one) zeros_range_size = VarintDecode(reader, decode_offset) + 1; else is_first_one = false;
      uint64_t ones_range_size = VarintDecode(reader, decode_offset) + 1;
      sum += zeros_range_size;
      for (uint64_t i = sum; i < sum + ones_range_size; ++i) pos_ones.push_back(i);
      sum += ones_range_size;
    }
  } else if (enc_type == 3) {
    // Ranges-Arith encoding.

    // If bit vector starts with 1.
    bool is_first_one = ((header >> 2) & 1) == 1;
    uint64_t freqs0_cnt = header >> 3, freqs1_cnt = VarintDecode(reader, decode_offset);
    vector<uint32_t> distr_table0 = SerialFreqsToDistrTable(reader, decode_offset, freqs0_cnt);
    vector<uint32_t> distr_table1 = SerialFreqsToDistrTable(reader, decode_offset, freqs1_cnt);
    uint64_t cnt_elements0 = VarintDecode(reader, decode_offset), cnt_elements1 = VarintDecode(reader, decode_offset);
    uint64_t enc0_sizes_bytesize = VarintDecode(reader, decode_offset), enc1_sizes_bytesize = VarintDecode(reader, decode_offset);
    Reader * arith_dec0_reader = reader.CreateSubReader(decode_offset, enc0_sizes_bytesize);
    ArithmeticDecoder arith_dec0(*arith_dec0_reader, distr_table0);
    vector<uint32_t> bits_sizes0;
    for (uint64_t i = 0; i < cnt_elements0; ++i) bits_sizes0.push_back(arith_dec0.Decode());
    decode_offset += enc0_sizes_bytesize;
    Reader * arith_dec1_reader = reader.CreateSubReader(decode_offset, enc1_sizes_bytesize);
    ArithmeticDecoder arith_dec1(*arith_dec1_reader, distr_table1);
    vector<uint32_t> bits_sizes1;
    for (uint64_t i = 0; i < cnt_elements1; ++i) bits_sizes1.push_back(arith_dec1.Decode());
    decode_offset += enc1_sizes_bytesize;
    Reader * bit_reader_reader = reader.CreateSubReader(decode_offset, serial_size - decode_offset);
    BitReader bit_reader(*bit_reader_reader);
    uint64_t sum = 0, i0 = 0, i1 = 0;
    while (i0 < cnt_elements0 && i1 < cnt_elements1) {
      uint64_t zeros_range_size = 0;
      // Don't read zero range size for the first time if first bit is 1.
      if (!is_first_one) {
        uint32_t bits_used = bits_sizes0[i0];
        if (bits_used > 0) zeros_range_size = ((uint64_t(1) << (bits_used - 1)) | bit_reader.Read(bits_used - 1)) + 1; else zeros_range_size = 1;
        ++i0;
      } else is_first_one = false;
      uint64_t ones_range_size = 0;
      uint32_t bits_used = bits_sizes1[i1];
      if (bits_used > 0) ones_range_size = ((uint64_t(1) << (bits_used - 1)) | bit_reader.Read(bits_used - 1)) + 1; else ones_range_size = 1;
      ++i1;
      sum += zeros_range_size;
      for (uint64_t j = sum; j < sum + ones_range_size; ++j) pos_ones.push_back(j);
      sum += ones_range_size;
    }
    ASSERT(i0 == cnt_elements0 && i1 == cnt_elements1, ());
    decode_offset = serial_size;
  }
  return pos_ones;
}
