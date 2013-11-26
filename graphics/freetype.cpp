#include "freetype.hpp"

#undef __FTERRORS_H__
#define FT_ERRORDEF(e, v, s) {e, s},
#define FT_ERROR_START_LIST  {
#define FT_ERROR_END_LIST    {0, 0}};
FreetypeError g_FT_Errors[] =
#include FT_ERRORS_H
