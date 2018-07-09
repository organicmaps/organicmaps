#include "codearea.h"

#include <algorithm>

namespace openlocationcode {

const double kLatitudeMaxDegrees = 90;
const double kLongitudeMaxDegrees = 180;

CodeArea::CodeArea(double latitude_lo, double longitude_lo, double latitude_hi,
                   double longitude_hi, size_t code_length) {
  latitude_lo_ = latitude_lo;
  longitude_lo_ = longitude_lo;
  latitude_hi_ = latitude_hi;
  longitude_hi_ = longitude_hi;
  code_length_ = code_length;
}

double CodeArea::GetLatitudeLo() const{
  return latitude_lo_;
}

double CodeArea::GetLongitudeLo() const {
  return longitude_lo_;
}

double CodeArea::GetLatitudeHi() const {
  return latitude_hi_;
}

double CodeArea::GetLongitudeHi() const {
  return longitude_hi_;
}

size_t CodeArea::GetCodeLength() const { return code_length_; }

LatLng CodeArea::GetCenter() const {
  double latitude_center = std::min(latitude_lo_ + (latitude_hi_ - latitude_lo_) / 2, kLatitudeMaxDegrees);
  double longitude_center = std::min(longitude_lo_ + (longitude_hi_ - longitude_lo_) / 2, kLongitudeMaxDegrees);
  LatLng center{latitude_center, longitude_center};
  return center;
}

} // namespace openlocationcode
