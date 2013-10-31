#pragma once

#include "../std/map.hpp"

namespace gpu
{
  struct ProgramInfo
  {
    ProgramInfo();
    ProgramInfo(int vertexIndex, int fragmentIndex,
                const char * vertexSource, const char * fragmentSource);
    int m_vertexIndex;
    int m_fragmentIndex;
    const char * m_vertexSource;
    const char * m_fragmentSource;
  };

  extern const int SOLID_AREA_PROGRAM;
  extern const int TEXTURING_PROGRAM;

  void InitGpuProgramsLib(map<int, ProgramInfo> & gpuIndex);
}
