#ifndef LOCATION_OPENLOCATIONCODE_CODEAREA_H_
#define LOCATION_OPENLOCATIONCODE_CODEAREA_H_

#include <cstdlib>

namespace openlocationcode {

struct LatLng {
  double latitude;
  double longitude;
};

class CodeArea {
 public:
  CodeArea(double latitude_lo, double longitude_lo, double latitude_hi,
           double longitude_hi, size_t code_length);
  double GetLatitudeLo() const;
  double GetLongitudeLo() const;
  double GetLatitudeHi() const;
  double GetLongitudeHi() const;
  size_t GetCodeLength() const;
  LatLng GetCenter() const;

 private:
  double latitude_lo_;
  double longitude_lo_;
  double latitude_hi_;
  double longitude_hi_;
  size_t code_length_;
};

}  // namespace openlocationcode

#endif  // LOCATION_OPENLOCATIONCODE_CODEAREA_H_
