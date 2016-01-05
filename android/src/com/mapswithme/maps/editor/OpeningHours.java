package com.mapswithme.maps.editor;

import android.support.annotation.IntRange;

import com.mapswithme.maps.editor.data.Timespan;
import com.mapswithme.maps.editor.data.Timetable;

public final class OpeningHours
{
  private OpeningHours() {}

  static
  {
    nativeInit();
  }

  private static native void nativeInit();

  public static native Timetable[] nativeGetDefaultTimetables();

  public static native Timetable nativeGetComplementTimetable(Timetable[] timetableSet);

  public static native Timetable[] nativeAddTimetable(Timetable[] timetableSet);

  public static native Timetable[] nativeRemoveTimetable(Timetable[] timetableSet, int timetableIndex);

  public static native Timetable nativeSetIsFullday(Timetable timetable, boolean isFullday);

  public static native Timetable[] nativeAddWorkingDay(Timetable[] timetables, int timetableIndex, @IntRange(from = 1, to = 7) int day);

  public static native Timetable[] nativeRemoveWorkingDay(Timetable[] timetables, int timetableIndex, @IntRange(from = 1, to = 7) int day);

  public static native Timetable nativeSetOpeningTime(Timetable timetable, Timespan openingTime);

  public static native Timetable nativeAddClosedSpan(Timetable timetable, Timespan closedSpan);

  public static native Timetable nativeRemoveClosedSpan(Timetable timetable, int spanIndex);

  public static native Timetable[] nativeTimetablesFromString(String source);

  public static native String nativeTimetablesToString(Timetable timetables[]);
}
