#pragma once

#include "indexer/feature_data.hpp"
#include "indexer/classificator.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "std/string.hpp"

struct MetadataTagProcessorImpl
{
  MetadataTagProcessorImpl(FeatureParams &params)
  : m_params(params)
  {
  }

  string ValidateAndFormat_maxspeed(string const & v) const;
  string ValidateAndFormat_stars(string const & v) const;
  string ValidateAndFormat_cuisine(string v) const;
  string ValidateAndFormat_operator(string const & v) const;
  string ValidateAndFormat_url(string const & v) const;
  string ValidateAndFormat_phone(string const & v) const;
  string ValidateAndFormat_opening_hours(string const & v) const;
  string ValidateAndFormat_ele(string const & v) const;
  string ValidateAndFormat_turn_lanes(string const & v) const;
  string ValidateAndFormat_turn_lanes_forward(string const & v) const;
  string ValidateAndFormat_turn_lanes_backward(string const & v) const;
  string ValidateAndFormat_email(string const & v) const;
  string ValidateAndFormat_postcode(string const & v) const;
  string ValidateAndFormat_flats(string const & v) const;
  string ValidateAndFormat_height(string const & v) const;
  string ValidateAndFormat_building_levels(string const & v) const;
  string ValidateAndFormat_denomination(string const & v) const;
  string ValidateAndFormat_wikipedia(string v) const;

protected:
  FeatureParams & m_params;
};

class MetadataTagProcessor : private MetadataTagProcessorImpl
{
public:
  using MetadataTagProcessorImpl::MetadataTagProcessorImpl;

  /// Since it is used as a functor wich stops iteration in ftype::ForEachTag
  /// and the is no need for interrupting it always returns false.
  /// TODO(mgsergio): Move to cpp after merge with https://github.com/mapsme/omim/pull/1314
  bool operator() (string const & k, string const & v)
  {
    if (v.empty())
      return false;

    using feature::Metadata;
    Metadata & md = m_params.GetMetadata();

    if (k == "cuisine")
    {
      string const & value = ValidateAndFormat_cuisine(v);
      if (!value.empty())
        md.Set(Metadata::FMD_CUISINE, value);
    }
    else if (k == "phone" || k == "contact:phone")
    {
      string const & value = ValidateAndFormat_phone(v);
      if (!value.empty())
        md.Set(Metadata::FMD_PHONE_NUMBER, value);
    }
    else if (k == "maxspeed")
    {
      string const & value = ValidateAndFormat_maxspeed(v);
      if (!value.empty())
        md.Set(Metadata::FMD_MAXSPEED, value);
    }
    else if (k == "stars")
    {
      string const & value = ValidateAndFormat_stars(v);
      if (!value.empty())
        md.Set(Metadata::FMD_STARS, value);
    }
    else if (k == "addr:postcode")
    {
      string const & value = ValidateAndFormat_postcode(v);
      if (!value.empty())
        md.Set(Metadata::FMD_POSTCODE, value);
    }
    else if (k == "url")
    {
      string const & value = ValidateAndFormat_url(v);
      if (!value.empty())
        md.Set(Metadata::FMD_URL, value);
    }
    else if (k == "website" || k == "contact:website")
    {
      string const & value = ValidateAndFormat_url(v);
      if (!value.empty())
        md.Set(Metadata::FMD_WEBSITE, value);
    }
    else if (k == "operator")
    {
      string const & value = ValidateAndFormat_operator(v);
      if (!value.empty())
        md.Set(Metadata::FMD_OPERATOR, value);
    }
    else if (k == "opening_hours")
    {
      string const & value = ValidateAndFormat_opening_hours(v);
      if (!value.empty())
        md.Set(Metadata::FMD_OPEN_HOURS, value);
    }
    else if (k == "ele")
    {
      string const & value = ValidateAndFormat_ele(v);
      if (!value.empty())
        md.Set(Metadata::FMD_ELE, value);
    }
    else if (k == "turn:lanes")
    {
      string const & value = ValidateAndFormat_turn_lanes(v);
      if (!value.empty())
        md.Set(Metadata::FMD_TURN_LANES, value);
    }
    else if (k == "turn:lanes:forward")
    {
      string const & value = ValidateAndFormat_turn_lanes_forward(v);
      if (!value.empty())
        md.Set(Metadata::FMD_TURN_LANES_FORWARD, value);
    }
    else if (k == "turn:lanes:backward")
    {
      string const & value = ValidateAndFormat_turn_lanes_backward(v);
      if (!value.empty())
        md.Set(Metadata::FMD_TURN_LANES_BACKWARD, value);
    }
    else if (k == "email" || k == "contact:email")
    {
      string const & value = ValidateAndFormat_email(v);
      if (!value.empty())
        md.Set(Metadata::FMD_EMAIL, value);
    }
    else if (k == "wikipedia")
    {
      string const & value = ValidateAndFormat_wikipedia(v);
      if (!value.empty())
        md.Set(Metadata::FMD_WIKIPEDIA, value);
    }
    else if (k == "addr:flats")
    {
      string const & value = ValidateAndFormat_flats(v);
      if (!value.empty())
        md.Set(Metadata::FMD_FLATS, value);
    }
    else if (k == "height")
    {
      string const & value = ValidateAndFormat_height(v);
      if (!value.empty())
        md.Set(Metadata::FMD_HEIGHT, value);
    }
    else if (k == "building:levels")
    {
      // Ignoring if FMD_HEIGHT already set
      if (md.Get(Metadata::FMD_HEIGHT).empty())
      {
        // Converting this attribute into height
        string const & value = ValidateAndFormat_building_levels(v);
        if (!value.empty())
          md.Set(Metadata::FMD_HEIGHT, value);
      }
    }
    else if (k == "min_height")
    {
      string const & value = ValidateAndFormat_height(v);
      if (!value.empty())
        md.Set(Metadata::FMD_MIN_HEIGHT, value);
    }
    else if (k == "building:min_level")
    {
      // Ignoring if min_height was already set
      if (md.Get(Metadata::FMD_MIN_HEIGHT).empty())
      {
        // Converting this attribute into height
        string const & value = ValidateAndFormat_building_levels(v);
        if (!value.empty())
          md.Set(Metadata::FMD_MIN_HEIGHT, value);
      }
    }
    else if (k == "denomination")
    {
      string const & value = ValidateAndFormat_denomination(v);
      if (!value.empty())
        md.Set(Metadata::FMD_DENOMINATION, value);
    }

    return false;
  }
};
