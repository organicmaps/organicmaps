#include "defines.hpp"
#include "../base/macros.hpp"
#include "../base/logging.hpp"
#include "../std/string.hpp"

namespace graphics
{
  namespace {
   struct Data
   {
     ESemantic m_val;
     char const * m_name;
   };
  }

  Data s_semantics[] = {
    {ESemPosition, "Position"},
    {ESemNormal, "Normal"},
    {ESemTexCoord0, "TexCoord0"},
    {ESemSampler0, "Sampler0"},
    {ESemModelView, "ModelView"},
    {ESemProjection, "Projection"}
  };

  void convert(char const * name, ESemantic & sem)
  {
    for (unsigned i = 0; i < ARRAY_SIZE(s_semantics); ++i)
    {
      if (strcmp(name, s_semantics[i].m_name) == 0)
      {
        sem = s_semantics[i].m_val;
        return;
      }
    }

    LOG(LINFO, ("Unknown Semantics=", name, "specified!"));
  }
}
