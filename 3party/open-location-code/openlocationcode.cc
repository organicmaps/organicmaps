#include "openlocationcode.h"

#include <ctype.h>
#include <float.h>
#include <algorithm>
#include <cmath>

#include "codearea.h"

namespace openlocationcode {
namespace internal {
const char kSeparator = '+';
const size_t kSeparatorPosition = 8;
const size_t kMaximumDigitCount = 32;
const char kPaddingCharacter = '0';
const char kAlphabet[] = "23456789CFGHJMPQRVWX";
const size_t kEncodingBase = 20;
const size_t kPairCodeLength = 10;
const size_t kGridColumns = 4;
const size_t kGridRows = kEncodingBase / kGridColumns;
const double kMinShortenDegrees = 0.05;
// Work out the encoding base exponent necessary to represent 360 degrees.
const size_t kInitialExponent = floor(log(360) / log(kEncodingBase));
// Work out the enclosing resolution (in degrees) for the grid algorithm.
const double kGridSizeDegrees =
    1 / pow(kEncodingBase, kPairCodeLength / 2 - (kInitialExponent + 1));

// Latitude bounds are -kLatitudeMaxDegrees degrees and +kLatitudeMaxDegrees
// degrees which we transpose to 0 and 180 degrees.
const double kLatitudeMaxDegrees = 90;
// Longitude bounds are -kLongitudeMaxDegrees degrees and +kLongitudeMaxDegrees
// degrees which we transpose to 0 and 360.
const double kLongitudeMaxDegrees = 180;
}  // namespace internal

namespace {

// Raises a number to an exponent, handling negative exponents.
double pow_neg(double base, double exponent) {
  if (exponent == 0) {
    return 1;
  } else if (exponent > 0) {
    return pow(base, exponent);
  }
  return 1 / pow(base, fabs(exponent));
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

// Finds the position of a char in the encoding alphabet.
int get_alphabet_position(char c) {
  const char* end = internal::kAlphabet + internal::kEncodingBase;
  const char* match = std::find(internal::kAlphabet, end, c);
  return (end == match)? -1 : (match - internal::kAlphabet);
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


// Encodes positive range lat,lng into a sequence of OLC lat/lng pairs.
// This uses pairs of characters (latitude and longitude in that order) to
// represent each step in a 20x20 grid. Each code, therefore, has 1/400th
// the area of the previous code.
std::string EncodePairs(double lat, double lng, size_t code_length) {
  std::string code;
  code.reserve(code_length + 1);
  // Provides the value of digits in this place in decimal degrees.
  double resolution_degrees = pow(
      internal::kEncodingBase, internal::kInitialExponent);
  // Add two digits on each pass.
  for (size_t digit_count = 0; digit_count < code_length;
      digit_count+=2, resolution_degrees/=internal::kEncodingBase) {
    // Do the latitude - gets the digit for this place and subtracts that for
    // the next digit.
    size_t digit_value = floor(lat / resolution_degrees);
    lat -= digit_value * resolution_degrees;
    code += internal::kAlphabet[digit_value];
    // And do the longitude - gets the digit for this place and subtracts that
    // for the next digit.
    digit_value = floor(lng / resolution_degrees);
    lng -= digit_value * resolution_degrees;
    code += internal::kAlphabet[digit_value];
    // Should we add a separator here?
    if (code.size() == internal::kSeparatorPosition &&
        code.size() < code_length) {
      code += internal::kSeparator;
    }
  }
  while (code.size() < internal::kSeparatorPosition) {
    code += internal::kPaddingCharacter;
  }
  if (code.size() == internal::kSeparatorPosition) {
    code += internal::kSeparator;
  }
  return code;
}


// Encodes a location using the grid refinement method into an OLC string.
// The grid refinement method divides the area into a grid of 4x5, and uses a
// single character to refine the area. The grid squares use the OLC characters
// in order to number the squares as follows:
//   R V W X
//   J M P Q
//   C F G H
//   6 7 8 9
//   2 3 4 5
// This allows default accuracy OLC codes to be refined with just a single
// character.
std::string EncodeGrid(double lat, double lng, size_t code_length) {
  std::string code;
  code.reserve(code_length + 1);
  double lat_grid_size = internal::kGridSizeDegrees;
  double lng_grid_size = internal::kGridSizeDegrees;
  // To avoid problems with floating point, get rid of the degrees.
  lat = fmod(lat, 1);
  lng = fmod(lng, 1);
  lat = fmod(lat, lat_grid_size);
  lng = fmod(lng, lng_grid_size);
  for (size_t i = 0; i < code_length; i++) {
    // The following clause should never execute because of maximum code length
    // enforcement in other functions, but is here to prevent division-by-zero
    // crash from underflow.
    if (lat_grid_size / internal::kGridRows <= DBL_MIN ||
        lng_grid_size / internal::kGridColumns <= DBL_MIN) {
      continue;
    }
    // Work out the row and column.
    size_t row = floor(lat / (lat_grid_size / internal::kGridRows));
    size_t col = floor(lng / (lng_grid_size / internal::kGridColumns));
    lat_grid_size /= internal::kGridRows;
    lng_grid_size /= internal::kGridColumns;
    lat -= row * lat_grid_size;
    lng -= col * lng_grid_size;
    code += internal::kAlphabet[row * internal::kGridColumns + col];
  }
  return code;
}

}  // anonymous namespace

std::string Encode(const LatLng &location, size_t code_length) {
  // Limit the maximum number of digits in the code.
  code_length = std::min(code_length, internal::kMaximumDigitCount);
  // Adjust latitude and longitude so they fall into positive ranges.
  double latitude = adjust_latitude(location.latitude, code_length) +
                    internal::kLatitudeMaxDegrees;
  double longitude =
      normalize_longitude(location.longitude) + internal::kLongitudeMaxDegrees;
  std::string code = EncodePairs(
      latitude, longitude, std::min(code_length, internal::kPairCodeLength));
  // If the requested length indicates we want grid refined codes.
  if (code_length > internal::kPairCodeLength) {
    code += EncodeGrid(latitude, longitude,
        code_length - internal::kPairCodeLength);
  }
  return code;
}

std::string Encode(const LatLng &location) {
  return Encode(location, internal::kPairCodeLength);
}

CodeArea Decode(const std::string &code) {
  // Make a copy that doesn't have the separator and stops at the first padding
  // character.
  std::string clean_code(code);
  clean_code.erase(
      std::remove(clean_code.begin(), clean_code.end(), internal::kSeparator),
      clean_code.end());
  if (clean_code.find(internal::kPaddingCharacter)) {
    clean_code = clean_code.substr(0,
        clean_code.find(internal::kPaddingCharacter));
  }
  double resolution_degrees = internal::kEncodingBase;
  double latitude = 0.0;
  double longitude = 0.0;
  double latitude_high = 0.0;
  double longitude_high = 0.0;
  // Up to the first 10 characters are encoded in pairs. Subsequent characters
  // represent grid squares.
  for (size_t i = 0; i < std::min(clean_code.size(), internal::kPairCodeLength);
       i += 2, resolution_degrees /= internal::kEncodingBase) {
    // The character at i represents latitude. Retrieve it and convert to
    // degrees (positive range).
    double value = get_alphabet_position(toupper(clean_code[i]));
    value *= resolution_degrees;
    latitude += value;
    latitude_high = latitude + resolution_degrees;
    // Checks if there are no more characters.
    if (i == std::min(clean_code.size(), internal::kPairCodeLength)) {
      break;
    }
    // The character at i + 1 represents longitude. Retrieve it and convert to
    // degrees (positive range).
    value = get_alphabet_position(toupper(clean_code[i + 1]));
    value *= resolution_degrees;
    longitude += value;
    longitude_high = longitude + resolution_degrees;
  }
  if (clean_code.size() > internal::kPairCodeLength) {
    // Now do any grid square characters.
    // Adjust the resolution back a step because we need the resolution of the
    // entire grid, not a single grid square.
    resolution_degrees *= internal::kEncodingBase;
    // With a grid, the latitude and longitude resolutions are no longer equal.
    double latitude_resolution = resolution_degrees;
    double longitude_resolution = resolution_degrees;
    // Decode only up to the maximum digit count.
    for (size_t i = internal::kPairCodeLength;
         i < std::min(internal::kMaximumDigitCount, clean_code.size()); i++) {
      // Get the value of the character at i and convert it to the degree value.
      size_t value = get_alphabet_position(toupper(clean_code[i]));
      size_t row = value / internal::kGridColumns;
      size_t col = value % internal::kGridColumns;
      // Lat and lng grid sizes shouldn't underflow due to maximum code length
      // enforcement, but a hypothetical underflow won't cause fatal errors
      // here.
      latitude_resolution /= internal::kGridRows;
      longitude_resolution /= internal::kGridColumns;
      latitude += row * latitude_resolution;
      longitude += col * longitude_resolution;
      latitude_high = latitude + latitude_resolution;
      longitude_high = longitude + longitude_resolution;
    }
  }
  return CodeArea(latitude - internal::kLatitudeMaxDegrees,
                  longitude - internal::kLongitudeMaxDegrees,
                  latitude_high - internal::kLatitudeMaxDegrees,
                  longitude_high - internal::kLongitudeMaxDegrees,
                  CodeLength(code));
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
    return short_code;
  }
  // Ensure that latitude and longitude are valid.
  double latitude =
      adjust_latitude(reference_location.latitude, CodeLength(short_code));
  double longitude = normalize_longitude(reference_location.longitude);
  // Compute the number of digits we need to recover.
  size_t padding_length = internal::kSeparatorPosition -
      short_code.find(internal::kSeparator);
  // The resolution (height and width) of the padded area in degrees.
  double resolution = pow_neg(
      internal::kEncodingBase, 2.0 - (padding_length / 2.0));
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
  if (latitude + half_res < center_lat && center_lat - resolution > -internal::kLatitudeMaxDegrees) {
    // If the proposed code is more than half a cell north of the reference location,
    // it's too far, and the best match will be one cell south.
    center_lat -= resolution;
  } else if (latitude - half_res > center_lat && center_lat + resolution < internal::kLatitudeMaxDegrees) {
    // If the proposed code is more than half a cell south of the reference location,
    // it's too far, and the best match will be one cell north.
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
  // Make sure the code does not have too many digits in total.
  if (code.size() - 1 > internal::kMaximumDigitCount) {
    return false;
  }
  // Make sure the code does not have too many digits after the separator.
  // The number of digits is the length of the code, minus the position of the
  // separator, minus one because the separator position is zero indexed.
  if (code.size() - code.find(internal::kSeparator) - 1 >
      internal::kMaximumDigitCount - internal::kSeparatorPosition) {
    return false;
  }
  // Are there any invalid characters?
  for (char c : code) {
    if (c != internal::kSeparator && c != internal::kPaddingCharacter &&
        std::find(std::begin(internal::kAlphabet),
                  std::end(internal::kAlphabet),
                  (char)toupper(c)) == std::end(internal::kAlphabet)) {
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
  size_t firstLatValue = get_alphabet_position(toupper(code.at(0)));
  firstLatValue *= internal::kEncodingBase;
  if (firstLatValue >= internal::kLatitudeMaxDegrees * 2) {
    // The code would decode to a latitude of >= 90 degrees.
    return false;
  }
  if (code.size() > 1) {
    // Work out what the first longitude character indicates for longitude.
    size_t firstLngValue = get_alphabet_position(toupper(code.at(1)));
    firstLngValue *= internal::kEncodingBase;
    if (firstLngValue >= internal::kLongitudeMaxDegrees * 2) {
      // The code would decode to a longitude of >= 180 degrees.
      return false;
    }
  }
  return true;
}

size_t CodeLength(const std::string &code) {
  // Remove the separator and any padding characters.
  std::string clean_code(code);
  clean_code.erase(
      std::remove(clean_code.begin(), clean_code.end(), internal::kSeparator),
      clean_code.end());
  if (clean_code.find(internal::kPaddingCharacter)) {
    clean_code = clean_code.substr(
        0, clean_code.find(internal::kPaddingCharacter));
  }
  return clean_code.size();
}

}  // namespace openlocationcode
