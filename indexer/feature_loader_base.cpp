#include "../base/SRC_FIRST.hpp"

#include "feature_loader_base.hpp"
#include "feature_loader.hpp"
#include "feature_impl.hpp"

#include "old/feature_loader_101.hpp"

#include "../defines.hpp"

#include "../coding/byte_stream.hpp"

#include "../base/start_mem_debug.hpp"


namespace feature
{

////////////////////////////////////////////////////////////////////////////////////////////
// SharedLoadInfo implementation.
////////////////////////////////////////////////////////////////////////////////////////////

SharedLoadInfo::SharedLoadInfo(FilesContainerR const & cont, DataHeader const & header)
  : m_cont(cont), m_header(header)
{
}

SharedLoadInfo::ReaderT SharedLoadInfo::GetDataReader() const
{
  return m_cont.GetReader(DATA_FILE_TAG);
}

SharedLoadInfo::ReaderT SharedLoadInfo::GetGeometryReader(int ind) const
{
  return m_cont.GetReader(GetTagForIndex(GEOMETRY_FILE_TAG, ind));
}

SharedLoadInfo::ReaderT SharedLoadInfo::GetTrianglesReader(int ind) const
{
  return m_cont.GetReader(GetTagForIndex(TRIANGLE_FILE_TAG, ind));
}

LoaderBase * SharedLoadInfo::CreateLoader() const
{
  LoaderBase * p;

  switch (m_header.GetVersion())
  {
  case DataHeader::v1:
    p = new old_101::feature::LoaderImpl(*this);
    break;

  default:
    p = new LoaderCurrent(*this);
    break;
  }

  return p;
}


////////////////////////////////////////////////////////////////////////////////////////////
// LoaderBase implementation.
////////////////////////////////////////////////////////////////////////////////////////////

LoaderBase::LoaderBase(SharedLoadInfo const & info)
  : m_Info(info), m_pF(0), m_Data(0)
{
}

void LoaderBase::Deserialize(BufferT data)
{
  m_Data = data;
  m_CommonOffset = m_Header2Offset = 0;
  m_ptsSimpMask = 0;
}

uint32_t LoaderBase::CalcOffset(ArrayByteSource const & source) const
{
  return static_cast<uint32_t>(source.PtrC() - DataPtr());
}

}
