#pragma once

#include "../base/logging.hpp"

// Put all needed FT includes in one place
#include <ft2build.h>
#include FT_TYPES_H
#include FT_SYSTEM_H
#include FT_FREETYPE_H
#include FT_STROKER_H
#include FT_CACHE_H

struct FreetypeError
{
  int m_code;
  char const * m_message;
};

extern FreetypeError g_FT_Errors[];

#define FREETYPE_CHECK(x) \
  do \
  { \
    FT_Error const err = (x); \
    if (err) \
      LOG(LWARNING, ("Freetype:", g_FT_Errors[err].m_code, g_FT_Errors[err].m_message)); \
  } while (false)

#define FREETYPE_CHECK_RETURN(x, msg) \
  do \
  { \
    FT_Error const err = (x); \
    if (err) \
    { \
      LOG(LWARNING, ("Freetype", g_FT_Errors[err].m_code, g_FT_Errors[err].m_message, msg)); \
      return; \
    } \
  } while (false)
