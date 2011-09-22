#include "index.hpp"
#include "data_header.hpp"
#include "../platform/platform.hpp"
#include "../std/bind.hpp"

namespace
{
  FilesContainerR * CreateFileContainer(string const & fileName)
  {
    return new FilesContainerR(GetPlatform().GetReader(fileName));
  }
}  // unnamed namespace

Index::Index() : MwmSet(bind(&Index::FillInMwmInfo, this, _1, _2), &CreateFileContainer)
{
}

void Index::FillInMwmInfo(string const & fileName, MwmInfo & info)
{
  IndexFactory factory;
  factory.Load(FilesContainerR(GetPlatform().GetReader(fileName)));

  feature::DataHeader const & h = factory.GetHeader();
  info.m_limitRect = h.GetBounds();

  pair<int, int> const scaleR = h.GetScaleRange();
  info.m_minScale = static_cast<uint8_t>(scaleR.first);
  info.m_maxScale = static_cast<uint8_t>(scaleR.second);
}
