#include "openlocationcode.h"

#include <float.h>

#include <algorithm>
#include <cmath>

#include "codearea.h"

namespace openlocationcode {
namespace internal {
const char kSeparator = '+';
const char kPaddingCharacter = '0';
const char kAlphabet[] = "23456789CFGHJMPQRVWX";
// Number of digits in the alphabet.
const size_t kEncodingBase = 20;
// The max number of digits returned in a plus code. Roughly 1 x 0.5 cm.
const size_t kMaximumDigitCount = 15;
const size_t kPairCodeLength = 10;
const size_t kGridCodeLength = kMaximumDigitCount - kPairCodeLength;
const size_t kGridColumns = 4;
const size_t kGridRows = kEncodingBase / kGridColumns;
const size_t kSeparatorPosition = 8;
// Work out the encoding base exponent necessary to represent 360 degrees.
const size_t kInitialExponent = floor(log(360) / log(kEncodingBase));
// Work out the enclosing resolution (in degrees) for the grid algorithm.
const double kGridSizeDegrees =
    1 / pow(kEncodingBase, kPairCodeLength / 2 - (kInitialExponent + 1));
// Inverse (1/) of the precision of the final pair digits in degrees. (20^3)
const size_t kPairPrecisionInverse = 8000;
// Inverse (1/) of the precision of the final grid digits in degrees.
// (Latitude and longitude are different.)
const size_t kGridLatPrecisionInverse =
    kPairPrecisionInverse * pow(kGridRows, kGridCodeLength);
const size_t kGridLngPrecisionInverse =
    kPairPrecisionInverse * pow(kGridColumns, kGridCodeLength);
// Latitude bounds are -kLatitudeMaxDegrees degrees and +kLatitudeMaxDegrees
// degrees which we transpose to 0 and 180 degrees.
const double kLatitudeMaxDegrees = 90;
// Longitude bounds are -kLongitudeMaxDegrees degrees and +kLongitudeMaxDegrees
// degrees which we transpose to 0 and 360.
const double kLongitudeMaxDegrees = 180;
// Lookup table of the alphabet positions of characters 'C' through 'X',
// inclusive. A value of -1 means the character isn't part of the alphabet.
const int kPositionLUT['X' - 'C' + 1] = {8,  -1, -1, 9,  10, 11, -1, 12,
                                         -1, -1, 13, -1, -1, 14, 15, 16,
                                         -1, -1, -1, 17, 18, 19};
}  // namespace internal

namespace {

// Raises a number to an exponent, handling negative exponents.
double pow_neg(double base, double exponent) {
  if (exponent == 0) {
    return 1;
  } else if (exponent > 0) {
    return pow(base, exponent);
  }
  return 1 / pow(base, -exponent);
}

// Compute the latitude precision value for a given code length. Lengths <= 10
// have the same precision for latitude and longitude, but lengths > 10 have
// different precisions due to the grid method having fewer columns than rows.
double compute_precision_for_length(int code_length) {
  if (code_length <= 10) {
    return pow_neg(internal::kEncodingBase, floor((code_length / -2) + 2));
  }
  return pow_neg(internal::kEncodingBase, -3) / pow(5, code_length - 10);
}

// Returns the position of a char in the encoding alphabet, or -1 if invalid.
int get_alphabet_position(char c) {
  // We use a lookup table for performance reasons (e.g. over std::find).
  if (c >= 'C' && c <= 'X') return internal::kPositionLUT[c - 'C'];
  if (c >= 'c' && c <= 'x') return internal::kPositionLUT[c - 'c'];
  if (c >= '2' && c <= '9') return c - '2';
  return -1;
}

// Normalize a longitude into the range -180 to 180, not including 180.
double normalize_longitude(double longitude_degrees) {
  while (longitude_degrees < -internal::kLongitudeMaxDegrees) {
    longitude_degrees = longitude_degrees + 360;
  }
  while (longitude_degrees >= internal::kLongitudeMaxDegrees) {
    longitude_degrees = longitude_degrees - 360;
  }
  return longitude_degrees;
}

// Adjusts 90 degree latitude to be lower so that a legal OLC code can be
// generated.
double adjust_latitude(double latitude_degrees, size_t code_length) {
  latitude_degrees = std::min(90.0, std::max(-90.0, latitude_degrees));

  if (latitude_degrees < internal::kLatitudeMaxDegrees) {
    return latitude_degrees;
  }
  // Subtract half the code precision to get the latitude into the code
  // area.
  double precision = compute_precision_for_length(code_length);
  return latitude_degrees - precision / 2;
}

// Remove the separator and padding characters from the code.
std::string clean_code_chars(const std::string &code) {
  std::string clean_code(code);
  clean_code.erase(
      std::remove(clean_code.begin(), clean_code.end(), internal::kSeparator),
      clean_code.end());
  if (clean_code.find(internal::kPaddingCharacter)) {
    clean_code =
        clean_code.substr(0, clean_code.find(internal::kPaddingCharacter));
  }
  return clean_code;
}

}  // anonymous namespace

std::string Encode(const LatLng &location, size_t code_length) {
  // Limit the maximum number of digits in the code.
  code_length = std::min(code_length, internal::kMaximumDigitCount);
  // Adjust latitude and longitude so that they are normalized/clipped.
  double latitude = adjust_latitude(location.latitude, code_length);
  double longitude = normalize_longitude(location.longitude);
  // Reserve 15 characters for the code digits. The separator will be inserted
  // at the end.
  std::string code = "123456789abcdef";

  // Compute the code.
  // This approach converts each value to an integer after multiplying it by
  // the final precision. This allows us to use only integer operations, so
  // avoiding any accumulation of floating point representation errors.

  // Multiply values by their precision and convert to positive without any
  // floating point operations.
  int64_t lat_val =
      internal::kLatitudeMaxDegrees * internal::kGridLatPrecisionInverse;
  int64_t lng_val =
      internal::kLongitudeMaxDegrees * internal::kGridLngPrecisionInverse;
  lat_val += latitude * internal::kGridLatPrecisionInverse;
  lng_val += longitude * internal::kGridLngPrecisionInverse;

  size_t pos = internal::kMaximumDigitCount - 1;
  // Compute the grid part of the code if necessary.
  if (code_length > internal::kPairCodeLength) {
    for (size_t i = 0; i < internal::kGridCodeLength; i++) {
      int lat_digit = lat_val % internal::kGridRows;
      int lng_digit = lng_val % internal::kGridColumns;
      int ndx = lat_digit * internal::kGridColumns + lng_digit;
      code.replace(pos--, 1, 1, internal::kAlphabet[ndx]);
      // Note! Integer division.
      lat_val /= internal::kGridRows;
      lng_val /= internal::kGridColumns;
    }
  } else {
    lat_val /= pow(internal::kGridRows, internal::kGridCodeLength);
    lng_val /= pow(internal::kGridColumns, internal::kGridCodeLength);
  }
  pos = internal::kPairCodeLength - 1;
  // Compute the pair section of the code.
  for (size_t i = 0; i < internal::kPairCodeLength / 2; i++) {
    int lat_ndx = lat_val % internal::kEncodingBase;
    int lng_ndx = lng_val % internal::kEncodingBase;
    code.replace(pos--, 1, 1, internal::kAlphabet[lng_ndx]);
    code.replace(pos--, 1, 1, internal::kAlphabet[lat_ndx]);
    // Note! Integer division.
    lat_val /= internal::kEncodingBase;
    lng_val /= internal::kEncodingBase;
  }

  // Add the separator character.
  code.insert(internal::kSeparatorPosition, &(internal::kSeparator), 1);

  // If we don't need to pad the code, return the requested section.
  if (code_length >= internal::kSeparatorPosition) {
    return code.substr(0, code_length + 1);
  }
  // Add the required padding characters.
  for (size_t i = code_length; i < internal::kSeparatorPosition; i++) {
    code[i] = internal::kPaddingCharacter;
  }
  // Return the code up to and including the separator.
  return code.substr(0, internal::kSeparatorPosition + 1);
}

std::string Encode(const LatLng &location) {
  return Encode(location, internal::kPairCodeLength);
}

CodeArea Decode(const std::string &code) {
  std::string clean_code = clean_code_chars(code);
  // Constrain to the maximum length.
  if (clean_code.size() > internal::kMaximumDigitCount) {
    clean_code = clean_code.substr(0, internal::kMaximumDigitCount);
  }
  // Initialise the values for each section. We work them out as integers and
  // convert them to floats at the end.
  int normal_lat =
      -internal::kLatitudeMaxDegrees * internal::kPairPrecisionInverse;
  int normal_lng =
      -internal::kLongitudeMaxDegrees * internal::kPairPrecisionInverse;
  int extra_lat = 0;
  int extra_lng = 0;
  // How many digits do we have to process?
  size_t digits = std::min(internal::kPairCodeLength, clean_code.size());
  // Define the place value for the most significant pair.
  int pv = pow(internal::kEncodingBase, internal::kPairCodeLength / 2 - 1);
  for (size_t i = 0; i < digits - 1; i += 2) {
    normal_lat += get_alphabet_position(clean_code[i]) * pv;
    normal_lng += get_alphabet_position(clean_code[i + 1]) * pv;
    if (i < digits - 2) {
      pv /= internal::kEncodingBase;
    }
  }
  // Convert the place value to a float in degrees.
  double lat_precision = (double)pv / internal::kPairPrecisionInverse;
  double lng_precision = (double)pv / internal::kPairPrecisionInverse;
  // Process any extra precision digits.
  if (clean_code.size() > internal::kPairCodeLength) {
    // Initialise the place values for the grid.
    int row_pv = pow(internal::kGridRows, internal::kGridCodeLength - 1);
    int col_pv = pow(internal::kGridColumns, internal::kGridCodeLength - 1);
    // How many digits do we have to process?
    digits = std::min(internal::kMaximumDigitCount, clean_code.size());
    for (size_t i = internal::kPairCodeLength; i < digits; i++) {
      int dval = get_alphabet_position(clean_code[i]);
      int row = dval / internal::kGridColumns;
      int col = dval % internal::kGridColumns;
      extra_lat += row * row_pv;
      extra_lng += col * col_pv;
      if (i < digits - 1) {
        row_pv /= internal::kGridRows;
        col_pv /= internal::kGridColumns;
      }
    }
    // Adjust the precisions from the integer values to degrees.
    lat_precision = (double)row_pv / internal::kGridLatPrecisionInverse;
    lng_precision = (double)col_pv / internal::kGridLngPrecisionInverse;
  }
  // Merge the values from the normal and extra precision parts of the code.
  // Everything is ints so they all need to be cast to floats.
  double lat = (double)normal_lat / internal::kPairPrecisionInverse +
               (double)extra_lat / internal::kGridLatPrecisionInverse;
  double lng = (double)normal_lng / internal::kPairPrecisionInverse +
               (double)extra_lng / internal::kGridLngPrecisionInverse;
  // Round everything off to 14 places.
  return CodeArea(round(lat * 1e14) / 1e14, round(lng * 1e14) / 1e14,
                  round((lat + lat_precision) * 1e14) / 1e14,
                  round((lng + lng_precision) * 1e14) / 1e14,
                  clean_code.size());
}

std::string Shorten(const std::string &code, const LatLng &reference_location) {
  if (!IsFull(code)) {
    return code;
  }
  if (code.find(internal::kPaddingCharacter) != std::string::npos) {
    return code;
  }
  CodeArea code_area = Decode(code);
  LatLng center = code_area.GetCenter();
  // Ensure that latitude and longitude are valid.
  double latitude =
      adjust_latitude(reference_location.latitude, CodeLength(code));
  double longitude = normalize_longitude(reference_location.longitude);
  // How close are the latitude and longitude to the code center.
  double range = std::max(fabs(center.latitude - latitude),
                          fabs(center.longitude - longitude));
  std::string code_copy(code);
  const double safety_factor = 0.3;
  const int removal_lengths[3] = {8, 6, 4};
  for (int removal_length : removal_lengths) {
    // Check if we're close enough to shorten. The range must be less than 1/2
    // the resolution to shorten at all, and we want to allow some safety, so
    // use 0.3 instead of 0.5 as a multiplier.
    double area_edge =
        compute_precision_for_length(removal_length) * safety_factor;
    if (range < area_edge) {
      code_copy = code_copy.substr(removal_length);
      break;
    }
  }
  return code_copy;
}

std::string RecoverNearest(const std::string &short_code,
                           const LatLng &reference_location) {
  if (!IsShort(short_code)) {
    std::string code = short_code;
    std::transform(code.begin(), code.end(), code.begin(), ::toupper);
    return code;
  }
  // Ensure that latitude and longitude are valid.
  double latitude =
      adjust_latitude(reference_location.latitude, CodeLength(short_code));
  double longitude = normalize_longitude(reference_location.longitude);
  // Compute the number of digits we need to recover.
  size_t padding_length =
      internal::kSeparatorPosition - short_code.find(internal::kSeparator);
  // The resolution (height and width) of the padded area in degrees.
  double resolution =
      pow_neg(internal::kEncodingBase, 2.0 - (padding_length / 2.0));
  // Distance from the center to an edge (in degrees).
  double half_res = resolution / 2.0;
  // Use the reference location to pad the supplied short code and decode it.
  LatLng latlng = {latitude, longitude};
  std::string padding_code = Encode(latlng);
  CodeArea code_rect =
      Decode(std::string(padding_code.substr(0, padding_length)) +
             std::string(short_code));
  // How many degrees latitude is the code from the reference? If it is more
  // than half the resolution, we need to move it north or south but keep it
  // within -90 to 90 degrees.
  double center_lat = code_rect.GetCenter().latitude;
  double center_lng = code_rect.GetCenter().longitude;
  if (latitude + half_res < center_lat &&
      center_lat - resolution > -internal::kLatitudeMaxDegrees) {
    // If the proposed code is more than half a cell north of the reference
    // location, it's too far, and the best match will be one cell south.
    center_lat -= resolution;
  } else if (latitude - half_res > center_lat &&
             center_lat + resolution < internal::kLatitudeMaxDegrees) {
    // If the proposed code is more than half a cell south of the reference
    // location, it's too far, and the best match will be one cell north.
    center_lat += resolution;
  }
  // How many degrees longitude is the code from the reference?
  if (longitude + half_res < center_lng) {
    center_lng -= resolution;
  } else if (longitude - half_res > center_lng) {
    center_lng += resolution;
  }
  LatLng center_latlng = {center_lat, center_lng};
  return Encode(center_latlng, CodeLength(short_code) + padding_length);
}

bool IsValid(const std::string &code) {
  if (code.empty()) {
    return false;
  }
  size_t separatorPos = code.find(internal::kSeparator);
  // The separator is required.
  if (separatorPos == std::string::npos) {
    return false;
  }
  // There must only be one separator.
  if (code.find_first_of(internal::kSeparator) !=
      code.find_last_of(internal::kSeparator)) {
    return false;
  }
  // Is the separator the only character?
  if (code.length() == 1) {
    return false;
  }
  // Is the separator in an illegal position?
  if (separatorPos > internal::kSeparatorPosition || separatorPos % 2 == 1) {
    return false;
  }
  // We can have an even number of padding characters before the separator,
  // but then it must be the final character.
  std::size_t paddingStart = code.find_first_of(internal::kPaddingCharacter);
  if (paddingStart != std::string::npos) {
    // Short codes cannot have padding
    if (separatorPos < internal::kSeparatorPosition) {
      return false;
    }
    // The first padding character needs to be in an odd position.
    if (paddingStart == 0 || paddingStart % 2) {
      return false;
    }
    // Padded codes must not have anything after the separator
    if (code.size() > separatorPos + 1) {
      return false;
    }
    // Get from the first padding character to the separator
    std::string paddingSection =
        code.substr(paddingStart, internal::kSeparatorPosition - paddingStart);
    paddingSection.erase(
        std::remove(paddingSection.begin(), paddingSection.end(),
                    internal::kPaddingCharacter),
        paddingSection.end());
    // After removing padding characters, we mustn't have anything left.
    if (!paddingSection.empty()) {
      return false;
    }
  }
  // If there are characters after the separator, make sure there isn't just
  // one of them (not legal).
  if (code.size() - code.find(internal::kSeparator) - 1 == 1) {
    return false;
  }
  // Are there any invalid characters?
  for (char c : code) {
    if (c != internal::kSeparator && c != internal::kPaddingCharacter &&
        get_alphabet_position(c) < 0) {
      return false;
    }
  }
  return true;
}

bool IsShort(const std::string &code) {
  // Check it's valid.
  if (!IsValid(code)) {
    return false;
  }
  // If there are less characters than expected before the SEPARATOR.
  if (code.find(internal::kSeparator) < internal::kSeparatorPosition) {
    return true;
  }
  return false;
}

bool IsFull(const std::string &code) {
  if (!IsValid(code)) {
    return false;
  }
  // If it's short, it's not full.
  if (IsShort(code)) {
    return false;
  }
  // Work out what the first latitude character indicates for latitude.
  size_t firstLatValue = get_alphabet_position(code.at(0));
  firstLatValue *= internal::kEncodingBase;
  if (firstLatValue >= internal::kLatitudeMaxDegrees * 2) {
    // The code would decode to a latitude of >= 90 degrees.
    return false;
  }
  if (code.size() > 1) {
    // Work out what the first longitude character indicates for longitude.
    size_t firstLngValue = get_alphabet_position(code.at(1));
    firstLngValue *= internal::kEncodingBase;
    if (firstLngValue >= internal::kLongitudeMaxDegrees * 2) {
      // The code would decode to a longitude of >= 180 degrees.
      return false;
    }
  }
  return true;
}

size_t CodeLength(const std::string &code) {
  std::string clean_code = clean_code_chars(code);
  return clean_code.size();
}

}  // namespace openlocationcode
