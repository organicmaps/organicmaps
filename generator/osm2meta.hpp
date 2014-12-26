#pragma once

#include "../indexer/feature_data.hpp"
#include "../indexer/classificator.hpp"
#include "../indexer/ftypes_matcher.hpp"

#include "../std/string.hpp"


class MetadataTagProcessor
{
  FeatureParams & m_params;

public:
  typedef bool result_type;

  MetadataTagProcessor(FeatureParams &params)
  : m_params(params)
  {
  }

  bool operator() (string const & k, string const & v)
  {

    if (v.empty())
      return false;

    if (k == "cuisine")
    {
      string const & value = ValidateAndFormat_cuisine(v);
      if (!value.empty())
        m_params.GetMetadata().Add(feature::FeatureMetadata::FMD_CUISINE, value);
    }
    else if (k == "phone")
    {
      string const & value = ValidateAndFormat_phone(v);
      if (!value.empty())
        m_params.GetMetadata().Add(feature::FeatureMetadata::FMD_PHONE_NUMBER, value);
    }
    else if (k == "stars")
    {
      string const & value = ValidateAndFormat_stars(v);
      if (!value.empty())
        m_params.GetMetadata().Add(feature::FeatureMetadata::FMD_STARS, value);
    }
    else if (k == "url")
    {
      string const & value = ValidateAndFormat_url(v);
      if (!value.empty())
        m_params.GetMetadata().Add(feature::FeatureMetadata::FMD_URL, value);
    }
    else if (k == "operator")
    {
      string const & value = ValidateAndFormat_operator(v);
      if (!value.empty())
        m_params.GetMetadata().Add(feature::FeatureMetadata::FMD_OPERATOR, value);
    }
    else if (k == "opening_hours")
    {
      string const & value = ValidateAndFormat_opening_hours(v);
      if (!value.empty())
      m_params.GetMetadata().Add(feature::FeatureMetadata::FMD_OPEN_HOURS, value);
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

};
