#pragma once

NEWTYPE(uint32_t, ObjectId);
NEWTYPE_SIMPLE_OUTPUT(ObjectId);

static double constexpr kDistanceLimitInMeters = 150;
static size_t constexpr kMaxSelectedElements = 3;
static ObjectId const kInvalidObjectId;

struct SponsoredObject
{
  enum class Fields
  {
    Id = 0,
    Latitude = 1,
    Longtitude = 2,
    Name = 3,
    Address = 4,
    Stars = 5,
    PriceCategory = 6,
    RatingBooking = 7,
    RatingUsers = 8,
    DescUrl = 9,
    Type = 10,
    Translations = 11,

    Counter
  };

  ObjectId id{kInvalidObjectId};
  double lat = 0.0;
  double lon = 0.0;
  string name;
  string address;
  string street;
  string houseNumber;
  uint32_t stars = 0;
  uint32_t priceCategory = 0;
  double ratingBooking = 0.0;
  double ratingUser = 0.0;
  string descUrl;
  uint32_t type = 0;
  string translations;

  static size_t constexpr Index(Fields field) { return static_cast<size_t>(field); }
  static size_t constexpr FieldsCount() { return static_cast<size_t>(Fields::Counter); }
  explicit SponsoredObject(string const & src);

  inline bool IsAddressFilled() const { return !street.empty() || !houseNumber.empty(); }
};

ostream & operator<<(ostream & s, BookingDataset::Hotel const & h);
