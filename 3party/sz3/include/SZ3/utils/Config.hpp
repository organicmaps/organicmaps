/**
 * @file Config.hpp
 * @brief This file defines the configuration class and related utilities for SZ3.
 *
 * @note Do not delete any existing fields for backward compatibility.
 *       These fields are critical for ensuring that serialized configurations
 *       from older versions of SZ3 remain compatible with newer versions.
 *       Removing or altering fields could break deserialization or cause
 *       unexpected behavior in applications relying on SZ3.
 */

#ifndef SZ3_Config_HPP
#define SZ3_Config_HPP

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <stdexcept>
#include <vector>

#include "SZ3/def.hpp"
#include "SZ3/utils/ByteUtil.hpp"
#include "SZ3/utils/MemoryUtil.hpp"
#include "SZ3/version.hpp"

#define SZ_FLOAT 0
#define SZ_DOUBLE 1
#define SZ_UINT8 2
#define SZ_INT8 3
#define SZ_UINT16 4
#define SZ_INT16 5
#define SZ_UINT32 6
#define SZ_INT32 7
#define SZ_UINT64 8
#define SZ_INT64 9

namespace SZ3 {

/**
 * @enum EB
 * @brief Enumeration for error bound modes.
 *
 * - EB_ABS: Absolute error bound. Limits the (de)compression errors to be within an absolute error. For example,
 * absErrorBound=0.0001 means the decompressed value must be in [V-0.0001,V+0.0001], where V is the original true value.
 * - EB_REL: Relative error bound ratio. Limits the (de)compression errors by considering the global data value range
 * size (max_value - min_value). For example, relErrorBound=0.01 means the error bound is 0.01 * (max - min).
 * - EB_PSNR: Peak Signal-to-Noise Ratio error bound. Expected PSNR. Only valid when ErrorBoundMode = PSNR.
 * - EB_L2NORM: L2 norm error bound. Expected L2 norm error: sqrt((x1-x1')^2 + (x2-x2')^2 + ... + (xN-xN')^2)
 * - EB_ABS_AND_REL: Combined absolute and relative error bound. Both absolute and relative bounds must be satisfied.
 * - EB_ABS_OR_REL: Either absolute or relative error bound. Either the absolute or relative bound must be satisfied
 * (whichever is looser).
 */
enum EB { EB_ABS, EB_REL, EB_PSNR, EB_L2NORM, EB_ABS_AND_REL, EB_ABS_OR_REL };

/**
 * @enum ALGO
 * @brief Enumeration for compression algorithms.
 *
 * - ALGO_LORENZO_REG: SZ2 algorithm (Lorenzo with regression).
 * - ALGO_INTERP_LORENZO: the default SZ3 algorithm (Interpolation with Lorenzo, with tuning).
 * - ALGO_INTERP: Interpolation-only (without tuning).
 * - ALGO_NOPRED: No prediction.
 * - ALGO_LOSSLESS: Lossless compression.
 * - ALGO_BIOMD: data compression algorithm for biology molecular data.
 * - ALGO_BIOMDXTC: removed in 3.4.0, kept so that code naming it compiles; compressing with it throws. Use ALGO_BIOMD.
 */
enum ALGO { ALGO_LORENZO_REG, ALGO_INTERP_LORENZO, ALGO_INTERP, ALGO_NOPRED, ALGO_LOSSLESS, ALGO_BIOMD, ALGO_BIOMDXTC };

/**
 * @enum INTERP_ALGO
 * @brief Enumeration for interpolation algorithms.
 *
 * - INTERP_ALGO_LINEAR: Linear interpolation.
 * - INTERP_ALGO_CUBIC: Cubic interpolation.
 */
enum INTERP_ALGO { INTERP_ALGO_LINEAR, INTERP_ALGO_CUBIC };

const std::map<std::string, ALGO> ALGO_MAP = {
    {"ALGO_LORENZO_REG", ALGO_LORENZO_REG}, {"ALGO_INTERP_LORENZO", ALGO_INTERP_LORENZO},
    {"ALGO_INTERP", ALGO_INTERP},           {"ALGO_NOPRED", ALGO_NOPRED},
    {"ALGO_LOSSLESS", ALGO_LOSSLESS},       {"ALGO_BIOMD", ALGO_BIOMD},
    {"ALGO_BIOMDXTC", ALGO_BIOMDXTC},
};

const std::map<std::string, EB> EB_MAP = {
    {"ABS", EB_ABS},
    {"REL", EB_REL},
    {"PSNR", EB_PSNR},
    {"NORM", EB_L2NORM},
    {"ABS_AND_REL", EB_ABS_AND_REL},
    {"ABS_OR_REL", EB_ABS_OR_REL},
};

const std::map<std::string, INTERP_ALGO> INTERP_ALGO_MAP = {
    {"INTERP_ALGO_LINEAR", INTERP_ALGO_LINEAR},
    {"INTERP_ALGO_CUBIC", INTERP_ALGO_CUBIC},
};

ALWAYS_INLINE std::string to_lower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(), ::tolower);
    return out;
}

template <typename EnumType>
void match_enum(const std::string& input, const std::map<std::string, EnumType>& table, uint8_t& out) {
    std::string input_lc = to_lower(input);
    for (const auto& [key, val] : table) {
        if (to_lower(key) == input_lc) {
            out = static_cast<int>(val);
        }
    }
}

template <typename EnumType>
std::string enum_to_string(EnumType value, const std::map<std::string, EnumType>& forward_map) {
    for (const auto& [str, val] : forward_map) {
        if (val == value) {
            return str;
        }
    }
    return "";
}

/**
 * @class Config
 * @brief Configuration class for SZ3.
 *
 * The Config class stores various parameters and settings used by SZ3, such as:
 * - Dimensions of the data.
 * - Compression algorithm and error bound settings.
 * - Module-specific parameters like interpolation settings.
 *
 * It provides methods to load/save configurations from/to INI files, serialize/deserialize configurations,
 * and estimate the size of the configuration.
 */
class Config {
   public:
    /**
     * @brief Constructor to initialize the configuration with dimensions.
     *
     * @tparam Dims Variadic template for dimension arguments.
     * @param args Dimensions of the data.
     */
    template <class... Dims>
    Config(Dims... args) {
        dims = std::vector<size_t>{static_cast<size_t>(std::forward<Dims>(args))...};
        setDims(dims.begin(), dims.end());
    }

    /**
     * @brief Set the dimensions of the data.
     *
     * @tparam Iter Iterator type for the dimension range.
     * @param begin Iterator to the beginning of the dimensions.
     * @param end Iterator to the end of the dimensions.
     * @return Total number of elements in the data.
     */
    template <class Iter>
    size_t setDims(Iter begin, Iter end) {
        auto dims_ = std::vector<size_t>(begin, end);
        dims.clear();
        for (auto dim : dims_) {
            if (dim > 1) {
                dims.push_back(dim);
            }
        }
        if (dims.empty()) {
            dims = {1};
        }
        N = dims.size();
        num = std::accumulate(dims.begin(), dims.end(), static_cast<size_t>(1), std::multiplies<size_t>());
        predDim = N;
        blockSize = (N == 1 ? 128 : (N == 2 ? 16 : 6));
        return num;
    }

    /**
     * @brief Load configuration from an INI file.
     *
     * @param ini_file_path Path to the INI file.
     * @throws std::runtime_error If the file cannot be opened.
     */
    void loadcfg(const std::string& ini_file_path) {
        std::ifstream file(ini_file_path);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open INI file: " + ini_file_path);
        }
        std::ostringstream buffer;
        buffer << file.rdbuf();
        load_ini(buffer.str());
    }

    /**
     * @brief Load configuration from an INI content string.
     *
     * @param ini_content Content of the INI file as a string.
     */
    void load_ini(const std::string& ini_content) {
        std::istringstream ss(ini_content);
        std::string line, section;

        auto trim = [](std::string& s) {
            s.erase(0, s.find_first_not_of(" \t\r\n"));
            s.erase(s.find_last_not_of(" \t\r\n") + 1);
        };

        auto eq = [&](const std::string& a, const std::string& b) { return to_lower(a) == to_lower(b); };

        auto parse_bool = [&](const std::string& s) {
            auto ls = to_lower(s);
            return ls == "true" || ls == "1" || ls == "yes" || ls == "on";
        };

        while (std::getline(ss, line)) {
            trim(line);
            if (line.empty() || line[0] == '#') continue;
            if (line.front() == '[') {
                section = line.substr(1, line.find(']') - 1);
                continue;
            }

            auto sep = line.find('=');
            if (sep == std::string::npos) continue;

            std::string key = line.substr(0, sep);
            std::string value = line.substr(sep + 1);
            trim(key);
            trim(value);

            if (eq(section, "GlobalSettings")) {
                if (eq(key, "CmprAlgo"))
                    match_enum(value, ALGO_MAP, cmprAlgo);
                else if (eq(key, "ErrorBoundMode"))
                    match_enum(value, EB_MAP, errorBoundMode);
                else if (eq(key, "AbsErrorBound"))
                    absErrorBound = std::stod(value);
                else if (eq(key, "RelErrorBound"))
                    relErrorBound = std::stod(value);
                else if (eq(key, "PSNRErrorBound"))
                    psnrErrorBound = std::stod(value);
                else if (eq(key, "L2NormErrorBound"))
                    l2normErrorBound = std::stod(value);
                else if (eq(key, "OpenMP"))
                    openmp = parse_bool(value);
            } else if (eq(section, "AlgoSettings")) {
                if (eq(key, "Lorenzo"))
                    lorenzo = parse_bool(value);
                else if (eq(key, "Lorenzo2ndOrder"))
                    lorenzo2 = parse_bool(value);
                else if (eq(key, "Regression"))
                    regression = parse_bool(value);
                else if (eq(key, "Regression2ndOrder"))
                    regression2 = parse_bool(value);
                else if (eq(key, "InterpolationAlgo"))
                    match_enum(value, INTERP_ALGO_MAP, interpAlgo);
                else if (eq(key, "InterpolationDirection"))
                    interpDirection = std::stoi(value);
                else if (eq(key, "BlockSize"))
                    blockSize = std::stoi(value);
                else if (eq(key, "QuantizationBinTotal"))
                    quantbinCnt = std::stoi(value);
                else if (eq(key, "InterpolationAnchorStride"))
                    interpAnchorStride = std::stoi(value);
                else if (eq(key, "InterpolationAlpha"))
                    interpAlpha = std::stod(value);
                else if (eq(key, "InterpolationBeta"))
                    interpBeta = std::stod(value);
            }
        }
    }

    /**
     * @brief Save the current configuration to an INI format string.
     *
     * @return INI format string representing the current configuration.
     */
    std::string save_ini() const {
        std::ostringstream ss;

        ss << "[GlobalSettings]\n";
        ss << "CmprAlgo = " << enum_to_string(static_cast<ALGO>(cmprAlgo), ALGO_MAP) << "\n";
        ss << "ErrorBoundMode = " << enum_to_string(static_cast<EB>(errorBoundMode), EB_MAP) << "\n";
        ss << "AbsErrorBound = " << absErrorBound << "\n";
        ss << "RelErrorBound = " << relErrorBound << "\n";
        ss << "PSNRErrorBound = " << psnrErrorBound << "\n";
        ss << "L2NormErrorBound = " << l2normErrorBound << "\n";
        ss << "OpenMP = " << (openmp ? "true" : "false") << "\n";

        ss << "\n[AlgoSettings]\n";
        ss << "Lorenzo = " << (lorenzo ? "true" : "false") << "\n";
        ss << "Lorenzo2ndOrder = " << (lorenzo2 ? "true" : "false") << "\n";
        ss << "Regression = " << (regression ? "true" : "false") << "\n";
        ss << "Regression2ndOrder = " << (regression2 ? "true" : "false") << "\n";
        ss << "BlockSize = " << blockSize << "\n";
        ss << "QuantizationBinTotal = " << quantbinCnt << "\n";
        ss << "InterpolationAlgo = " << enum_to_string(static_cast<INTERP_ALGO>(interpAlgo), INTERP_ALGO_MAP) << "\n";
        ss << "InterpolationDirection = " << static_cast<int>(interpDirection) << "\n";
        ss << "InterpolationAnchorStride = " << interpAnchorStride << "\n";
        ss << "InterpolationAlpha = " << interpAlpha << "\n";
        ss << "InterpolationBeta = " << interpBeta << "\n";
        return ss.str();
    }

    /**
     * @brief Serialize the configuration to a byte array.
     * @note some fields are saved/loaded in specific modules, not here.
     * @param c Pointer to the byte array.
     * @return Size of the serialized configuration.
     */
    size_t save(unsigned char*& c) const {
        auto c0 = c;
        c += sizeof(uchar);  // reserve space for conf size

        write(N, c);
        // write(dims.data(), dims.size(), c);
        auto bitWidth = vector_bit_width(dims);
        write(bitWidth, c);
        vector2bytes(dims, bitWidth, c);

        write<uint64_t>(num, c);
        write(cmprAlgo, c);

        write(errorBoundMode, c);
        if (errorBoundMode == EB_ABS) {
            write(absErrorBound, c);
        } else if (errorBoundMode == EB_REL) {
            write(relErrorBound, c);
        } else if (errorBoundMode == EB_PSNR) {
            write(psnrErrorBound, c);
        } else if (errorBoundMode == EB_L2NORM) {
            write(l2normErrorBound, c);
        } else if (errorBoundMode == EB_ABS_OR_REL) {
            write(absErrorBound, c);
            write(relErrorBound, c);
        } else if (errorBoundMode == EB_ABS_AND_REL) {
            write(absErrorBound, c);
            write(relErrorBound, c);
        } else {
            throw std::invalid_argument("SZ3 Config::save: unknown error bound mode");
        }

        uint8_t boolvals = (lorenzo & 1) << 7 | (lorenzo2 & 1) << 6 | (regression & 1) << 5 | (regression2 & 1) << 4 |
                           (openmp & 1) << 3;
        write(boolvals, c);

        write(dataType, c);
        write(quantbinCnt, c);
        write(blockSize, c);
        write(predDim, c);

        const size_t written = static_cast<size_t>(c - c0);
        if (written > std::numeric_limits<uchar>::max()) {
            throw std::length_error("SZ3 Config::save: a serialized config is at most " +
                                    std::to_string(std::numeric_limits<uchar>::max()) + " bytes, this one is " +
                                    std::to_string(written));
        }
        auto confSize = static_cast<uchar>(written);
        write(confSize, c0);  // write conf size at reserved space
        return confSize;
    }

    /**
     * @brief Deserialize the configuration from a byte array.
     *
     * @param c Pointer to the byte array.
     * @param remaining_length bytes readable from `c`; decremented by what is consumed.
     */
    void load(const unsigned char*& c, size_t& remaining_length) {
        const unsigned char* const c0 = c;
        const unsigned char* const cend = c + remaining_length;
        uchar confSize = 0;
        read(confSize, c, remaining_length);
        // Where the optional fields at the end stop. A writer that declares more than it sends stops
        // at the buffer instead.
        const unsigned char* const c1 = std::min(c0 + confSize, cend);

        read(N, c, remaining_length);
        // Not `N > 4`: char may be signed, and then a byte like 0xF3 arrives negative and passes.
        if (static_cast<unsigned char>(N) > 4)
            throw std::out_of_range("SZ3 Config::load: invalid number of dimensions");
        uint8_t bitWidth;
        read(bitWidth, c, remaining_length);
        if (bitWidth > 64) throw std::out_of_range("SZ3 Config::load: invalid dimension bit width");
        const size_t dim_bytes = (static_cast<size_t>(N) * bitWidth + 7) / 8;
        if (dim_bytes > remaining_length) throw std::out_of_range("SZ3 Config::load: dimensions exceed the buffer");
        dims = bytes2vector<size_t>(c, bitWidth, N);
        remaining_length -= dim_bytes;
        uint64_t num64 = 0;
        read(num64, c, remaining_length);
        num = num64;
        // num must equal the product of the dimensions, or the predictor walks more grid positions than
        // were allocated. No dimensions means no elements: the HDF5 filter loads a config from cd_values
        // before it knows the dataset shape.
        size_t dims_product = dims.empty() ? 0 : 1;
        bool dims_ok = true;
        for (size_t dim : dims) {
            if (dim == 0 || dims_product > std::numeric_limits<size_t>::max() / dim) {
                dims_ok = false;
                break;
            }
            dims_product *= dim;
        }
        if (!dims_ok || dims_product != num)
            throw std::out_of_range("SZ3 Config::load: dimensions inconsistent with the element count");
        read(cmprAlgo, c, remaining_length);

        read(errorBoundMode, c, remaining_length);
        if (errorBoundMode == EB_ABS) {
            read(absErrorBound, c, remaining_length);
        } else if (errorBoundMode == EB_REL) {
            read(relErrorBound, c, remaining_length);
        } else if (errorBoundMode == EB_PSNR) {
            read(psnrErrorBound, c, remaining_length);
        } else if (errorBoundMode == EB_L2NORM) {
            read(l2normErrorBound, c, remaining_length);
        } else if (errorBoundMode == EB_ABS_OR_REL || errorBoundMode == EB_ABS_AND_REL) {
            read(absErrorBound, c, remaining_length);
            read(relErrorBound, c, remaining_length);
        } else {
            // save() always writes a bound here, so an unhandled mode would shift every field below it.
            throw std::invalid_argument("SZ3 Config::load: unknown error bound mode");
        }

        if (c < c1) {
            uint8_t boolvals;
            read(boolvals, c, remaining_length);
            lorenzo = (boolvals >> 7) & 1;
            lorenzo2 = (boolvals >> 6) & 1;
            regression = (boolvals >> 5) & 1;
            regression2 = (boolvals >> 4) & 1;
            openmp = (boolvals >> 3) & 1;
        }
        if (c < c1) read(dataType, c, remaining_length);
        if (c < c1) read(quantbinCnt, c, remaining_length);
        if (c < c1) read(blockSize, c, remaining_length);
        if (c < c1) read(predDim, c, remaining_length);
        // Skip fields a later version appends, so whatever follows this config is read from where it starts.
        if (c > c1) throw std::out_of_range("SZ3 Config::load: the config is longer than its declared size");
        remaining_length -= static_cast<size_t>(c1 - c);
        c = c1;
    }

    /**
     * @brief Print the current configuration.
     */
    void print() {
        // printf("===================== Begin SZ3 Configuration =====================\n");
        printf("\nsz3MagicNumber = %u\n", sz3MagicNumber);
        printf("sz3DataVer = %s\n", versionStr(sz3DataVer).data());
        std::cout << "Dimensions =";
        for (auto d : dims) {
            std::cout << " " << d;
        }
        std::cout << "\n";
        std::cout << save_ini();
    }

    /**
     * @brief Estimate the size of the serialized configuration.
     *
     * @return Estimated size in bytes.
     */
    size_t size_est() const {
        std::vector<uchar> buffer(sizeof(Config) + 1024);
        auto buffer_pos = buffer.data();
        return save(buffer_pos);
    }

    uint32_t sz3MagicNumber = SZ3_MAGIC_NUMBER;      ///< SZ3 magic number for identification
    uint32_t sz3DataVer = versionInt(SZ3_DATA_VER);  ///< SZ3 data version

    char N = 0;                ///< Number of dimensions
    std::vector<size_t> dims;  ///< Dimensions of the data
    size_t num = 0;            ///< Total number of data points

    /**
     * @brief Global settings for compression.
     * These settings are generally applicable for all users.
     */
    uint8_t cmprAlgo = ALGO_INTERP_LORENZO;  ///< Compression algorithm
    uint8_t errorBoundMode = EB_ABS;         ///< Error bound mode
    double absErrorBound = 1e-3;             ///< Absolute error bound
    double relErrorBound = 0.0;              ///< Relative error bound
    double psnrErrorBound = 0.0;             ///< PSNR error bound
    double l2normErrorBound = 0.0;           ///< L2 norm error bound
    bool openmp = false;                     ///< Use OpenMP for parallel processing

    /**
     * @brief Algorithm-specific settings.
     * @note These are for advanced users or automatic tuning.
     * @note Newly added settings should be saved/loaded in specific modules, not here.
     */
    int quantbinCnt = 65536;                 ///< Maximum number of quantization intervals
    int blockSize = 0;                       ///< Block size for processing
    uint8_t predDim = 0;                     ///< Prediction dimension (currently unused)
    uint8_t dataType = SZ_FLOAT;             ///< Data type (used in HDF5 filter)
    bool lorenzo = true;                     ///< Enable 1st order Lorenzo
    bool lorenzo2 = false;                   ///< Enable 2nd order Lorenzo
    bool regression = true;                  ///< Enable 1st order regression
    bool regression2 = false;                ///< Enable 2nd order regression
    uint8_t interpAlgo = INTERP_ALGO_CUBIC;  ///< Interpolation algorithm. Saved/loaded in InterpolationDecomposition.
    uint8_t interpDirection = 0;             ///< Interpolation direction. Saved/loaded in InterpolationDecomposition.
    int interpAnchorStride =
        -1;  ///< Interpolation anchor stride (-1 for dynamic). Saved/loaded in InterpolationDecomposition.
    double interpAlpha = 1.25;  ///< Interpolation eb tuning parameter. Saved/loaded in InterpolationDecomposition.
    double interpBeta = 2.0;    ///< Interpolation eb tuning parameter. Saved/loaded in InterpolationDecomposition.
};

}  // namespace SZ3

#endif  // SZ_CONFIG_HPP
