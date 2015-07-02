#pragma once

#include "indexer/feature_data.hpp"
#include "indexer/classificator.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "base/string_utils.hpp"

#include "std/string.hpp"


class MetadataTagProcessor
{
  FeatureParams & m_params;

public:
  MetadataTagProcessor(FeatureParams &params)
  : m_params(params)
  {
  }

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
        md.Add(Metadata::FMD_CUISINE, value);
    }
    else if (k == "phone" || k == "contact:phone")
    {
      string const & value = ValidateAndFormat_phone(v);
      if (!value.empty())
        md.Add(Metadata::FMD_PHONE_NUMBER, value);
    }
    else if (k == "stars")
    {
      string const & value = ValidateAndFormat_stars(v);
      if (!value.empty())
        md.Add(Metadata::FMD_STARS, value);
    }
    else if (k == "addr:postcode")
    {
      string const & value = ValidateAndFormat_postcode(v);
      if (!value.empty())
        md.Add(Metadata::FMD_POSTCODE, value);
    }
    else if (k == "url")
    {
      string const & value = ValidateAndFormat_url(v);
      if (!value.empty())
        md.Add(Metadata::FMD_URL, value);
    }
    else if (k == "website" || k == "contact:website")
    {
      string const & value = ValidateAndFormat_url(v);
      if (!value.empty())
        md.Add(Metadata::FMD_WEBSITE, value);
    }
    else if (k == "operator")
    {
      string const & value = ValidateAndFormat_operator(v);
      if (!value.empty())
        md.Add(Metadata::FMD_OPERATOR, value);
    }
    else if (k == "opening_hours")
    {
      string const & value = ValidateAndFormat_opening_hours(v);
      if (!value.empty())
        md.Add(Metadata::FMD_OPEN_HOURS, value);
    }
    else if (k == "ele")
    {
      string const & value = ValidateAndFormat_ele(v);
      if (!value.empty())
        md.Add(Metadata::FMD_ELE, value);
    }
    else if (k == "turn:lanes")
    {
      string const & value = ValidateAndFormat_turn_lanes(v);
      if (!value.empty())
        md.Add(Metadata::FMD_TURN_LANES, value);
    }
    else if (k == "turn:lanes:forward")
    {
      string const & value = ValidateAndFormat_turn_lanes_forward(v);
      if (!value.empty())
        md.Add(Metadata::FMD_TURN_LANES_FORWARD, value);
    }
    else if (k == "turn:lanes:backward")
    {
      string const & value = ValidateAndFormat_turn_lanes_backward(v);
      if (!value.empty())
        md.Add(Metadata::FMD_TURN_LANES_BACKWARD, value);
    }
    else if (k == "email" || k == "contact:email")
    {
      string const & value = ValidateAndFormat_email(v);
      if (!value.empty())
        md.Add(Metadata::FMD_EMAIL, value);
    }
    return false;
  }

protected:
  /// Validation and formatting functions

  string ValidateAndFormat_stars(string const & v) const
  {
    if (v.empty())
      return string();

    // we accepting stars from 1 to 7
    if (v[0] <= '0' || v[0] > '7')
      return string();

    // ignore numbers large then 9
    if (v.size() > 1 && (v[1] >= '0' && v[1] <= '9'))
      return string();

    return string(1, v[0]);
  }

  string ValidateAndFormat_cuisine(string const & v) const
  {
    return v;
  }
  string ValidateAndFormat_operator(string const & v) const
  {
    static ftypes::IsATMChecker const IsATM;
    static ftypes::IsFuelStationChecker const IsFuelStation;

    if (!(IsATM(m_params.m_Types) || IsFuelStation(m_params.m_Types)))
      return string();

    return v;
  }
  string ValidateAndFormat_url(string const & v) const
  {
    return v;
  }
  string ValidateAndFormat_phone(string const & v) const
  {
    return v;
  }
  string ValidateAndFormat_opening_hours(string const & v) const
  {
    return v;
  }
  string ValidateAndFormat_ele(string const & v) const
  {
    static ftypes::IsPeakChecker const IsPeak;
    if (!IsPeak(m_params.m_Types))
      return string();
    double val = 0;
    if(!strings::to_double(v, val) || val == 0)
      return string();
    return v;
  }
  string ValidateAndFormat_turn_lanes(string const & v) const
  {
    return v;
  }
  string ValidateAndFormat_turn_lanes_forward(string const & v) const
  {
    return v;
  }
  string ValidateAndFormat_turn_lanes_backward(string const & v) const
  {
    return v;
  }
  string ValidateAndFormat_email(string const & v) const
  {
    return v;
  }
  string ValidateAndFormat_postcode(string const & v) const
  {
    return v;
  }

};
