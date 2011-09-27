#include "index.hpp"
#include "data_header.hpp"

#include "../platform/platform.hpp"


MwmValue::MwmValue(string const & name)
  : m_cont(GetPlatform().GetReader(name))
{
  m_factory.Load(m_cont);
}

void Index::GetInfo(string const & name, MwmInfo & info) const
{
  MwmValue value(name);

  feature::DataHeader const & h = value.GetHeader();
  info.m_limitRect = h.GetBounds();

  pair<int, int> const scaleR = h.GetScaleRange();
  info.m_minScale = static_cast<uint8_t>(scaleR.first);
  info.m_maxScale = static_cast<uint8_t>(scaleR.second);
}

MwmValue * Index::CreateValue(string const & name) const
{
  return new MwmValue(name);
}

Index::Index()
{
}

Index::~Index()
{
  Cleanup();
}

using namespace covering;

IntervalsT const & Index::CoveringGetter::Get(feature::DataHeader const & header)
{
  int const cellDepth = GetCodingDepth(header.GetScaleRange());
  int const ind = (cellDepth == RectId::DEPTH_LEVELS ? 0 : 1);

  if (m_res[ind].empty())
  {
    switch (m_mode)
    {
    case 0:
      CoverViewportAndAppendLowerLevels(m_rect, cellDepth, m_res[ind]);
      break;

    case 1:
      AppendLowerLevels(GetRectIdAsIs(m_rect), cellDepth, m_res[ind]);
      break;

    case 2:
      m_res[ind].push_back(IntervalsT::value_type(0, static_cast<int64_t>((1ULL << 63) - 1)));
      break;
    }
  }

  return m_res[ind];
}
