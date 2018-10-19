package com.mapswithme.maps.ugc.routes;

interface AdapterIndexConverter
{
  AdapterIndexAndPosition getRelativePosition(int absPosition);

  AdapterIndexAndViewType getRelativeViewType(int absViewType);

  int toAbsoluteViewType(AdapterIndexAndViewType type);
}
