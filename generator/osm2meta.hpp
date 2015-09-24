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
    else if (k == "wikipedia")
    {
      string const & value = ValidateAndFormat_wikipedia(v);
      if (!value.empty())
        md.Add(Metadata::FMD_WIKIPEDIA, value);
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

  string url_encode(string const & value) const
  {
    ostringstream escaped;
    escaped.fill('0');
    escaped << hex;

    for (string::const_iterator i = value.begin(), n = value.end(); i != n; ++i)
    {
      string::value_type c = (*i);

      // Keep alphanumeric and other accepted characters intact
      // Convert spaces to '_' as wikipedia does
      // Turn other characters to '%00' sequences
      if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
        escaped << c;
      else if (c == ' ')
        escaped << '_';
      else
        escaped << '%' << std::uppercase << setw(2) << int((unsigned char) c);
    }

    return escaped.str();
  }

  string ValidateAndFormat_wikipedia(string const & v) const
  {
    // Shortest string: "lg:aa"
    if (v.length() < 5)
      return v;

    // Find prefix before ':'
    int i = 0;
    while (i + 2 < v.length() && i < 10 && v[i] != ':')
      i++;
    if (v[i] != ':')
      return string();

    // URL encode lang:title, so URL can be reconstructed faster
    if (i <= 3 || v.substr(0, i) == "be-x-old")
      return v.substr(0, i + 1) + url_encode(v.substr(i + 1));

    if (v[i+1] == '/' && i + 27 < v.length())
    {
      // Convert URL to "lang:title"
      i += 3;
      int j = i;
      while (j < v.length() && v[j] != '.')
        j++;
      if (v.substr(j, 20) == ".wikipedia.org/wiki/")
        return v.substr(i, j - i) + ":" + v.substr(j + 20);
    }
    return string();
  }

};
