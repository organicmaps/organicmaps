#include "mwm_version.hpp"
#include "data_header.hpp"

#include "../coding/varint.hpp"
#include "../coding/writer.hpp"
#include "../coding/reader_wrapper.hpp"

#include "../base/timer.hpp"


namespace ver {

typedef feature::DataHeader FHeaderT;

char MWM_PROLOG[] = "MWM";

void WriteVersion(Writer & w)
{
  w.Write(MWM_PROLOG, ARRAY_SIZE(MWM_PROLOG));

  // write inner data version
  WriteVarUint(w, static_cast<uint32_t>(FHeaderT::lastVersion));

  // static is used for equal time stamp for all "mwm" files in one generation process
  static uint32_t generatorStartTime = my::TodayAsYYMMDD();
  WriteVarUint(w, generatorStartTime);
}

template <class TSource> uint32_t ReadVersionT(TSource & src)
{
  size_t const prologSize = ARRAY_SIZE(MWM_PROLOG);
  char prolog[prologSize];
  src.Read(prolog, prologSize);

  if (strcmp(prolog, MWM_PROLOG) != 0)
    return FHeaderT::v2;

  return ReadVarUint<uint32_t>(src);
}

uint32_t ReadVersion(ModelReaderPtr const & r)
{
  ReaderSource<ModelReaderPtr> src(r);
  return ReadVersionT(src);
}

uint32_t ReadTimestamp(ReaderSrc & src)
{
  (void)ReadVersionT(src);

  return ReadVarUint<uint32_t>(src);
}

}
