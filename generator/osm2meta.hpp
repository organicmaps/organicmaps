#pragma once

#include "indexer/feature_data.hpp"
#include "indexer/validate_and_format_contacts.hpp"

#include <string>

struct MetadataTagProcessorImpl
{
  MetadataTagProcessorImpl(FeatureBuilderParams & params) : m_params(params) {}

  std::string ValidateAndFormat_maxspeed(std::string const & v) const;
  static std::string ValidateAndFormat_stars(std::string const & v) ;
  std::string ValidateAndFormat_operator(std::string const & v) const;
  static std::string ValidateAndFormat_url(std::string const & v) ;
  static std::string ValidateAndFormat_phone(std::string const & v) ;
  static std::string ValidateAndFormat_opening_hours(std::string const & v) ;
  std::string ValidateAndFormat_ele(std::string const & v) const;
  static std::string ValidateAndFormat_destination(std::string const & v) ;
  static std::string ValidateAndFormat_destination_ref(std::string const & v) ;
  static std::string ValidateAndFormat_junction_ref(std::string const & v) ;
  static std::string ValidateAndFormat_turn_lanes(std::string const & v) ;
  static std::string ValidateAndFormat_turn_lanes_forward(std::string const & v) ;
  static std::string ValidateAndFormat_turn_lanes_backward(std::string const & v) ;
  static std::string ValidateAndFormat_email(std::string const & v) ;
  static std::string ValidateAndFormat_postcode(std::string const & v) ;
  static std::string ValidateAndFormat_flats(std::string const & v) ;
  static std::string ValidateAndFormat_internet(std::string v) ;
  static std::string ValidateAndFormat_height(std::string const & v) ;
  static std::string ValidateAndFormat_building_levels(std::string v) ;
  static std::string ValidateAndFormat_level(std::string v) ;
  static std::string ValidateAndFormat_denomination(std::string const & v) ;
  static std::string ValidateAndFormat_wikipedia(std::string v) ;
  static std::string ValidateAndFormat_wikimedia_commons(std::string v) ;
  std::string ValidateAndFormat_airport_iata(std::string const & v) const;
  std::string ValidateAndFormat_duration(std::string const & v) const;

protected:
  FeatureBuilderParams & m_params;
};

class MetadataTagProcessor : private MetadataTagProcessorImpl
{
  StringUtf8Multilang m_description;

public:
  /// Make base class constructor public.
  using MetadataTagProcessorImpl::MetadataTagProcessorImpl;

  // Assume that processor is created once, and we can make additional finalization in dtor.
  ~MetadataTagProcessor();

  void operator()(std::string const & k, std::string const & v);
};
