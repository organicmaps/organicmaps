#include "../base/SRC_FIRST.hpp"
#include "ft2_debug.hpp"

#undef __FTERRORS_H__
#define FT_ERRORDEF( e, v, s )  { e, s },
#define FT_ERROR_START_LIST     {
#define FT_ERROR_END_LIST       { 0, 0 } };

const struct
{
  int          err_code;
  const char*  err_msg;
} ft_errors[] =

#include FT_ERRORS_H

char const * FT_Error_Description(FT_Error error)
{
  int i = 1;
  while (ft_errors[i].err_code != 0)
  {
    if (ft_errors[i].err_code == error)
      break;
    else
      ++i;
  }
  return ft_errors[i].err_msg;
}

void CheckError(FT_Error error)
{
  if (error != 0)
    LOG(LINFO, ("FT_Error : ", FT_Error_Description(error)));
}

