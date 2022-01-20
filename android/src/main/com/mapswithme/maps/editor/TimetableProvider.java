package com.mapswithme.maps.editor;

import androidx.annotation.Nullable;

interface TimetableProvider
{
  @Nullable String getTimetables();
  void setTimetables(@Nullable String timetables);
}
