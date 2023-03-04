package app.organicmaps.scheduling;

import app.organicmaps.background.OsmUploadWork;

import java.util.HashMap;
import java.util.Map;

public class JobIdMap
{
  private static final Map<Class<?>, Integer> MAP = new HashMap<>();

  static {
    MAP.put(OsmUploadWork.class, calcIdentifier(MAP.size()));
  }

  private static final int ID_BASIC = 1070;
  private static final int JOB_TYPE_SHIFTS = 12;

  private static int calcIdentifier(int count)
  {
    return (count + 1 << JOB_TYPE_SHIFTS) + ID_BASIC;
  }

  public static Integer getId(Class<?> clazz)
  {
    Integer integer = MAP.get(clazz);
    if (integer == null)
      throw new IllegalArgumentException("Value not found for args : " + clazz);
    return integer;
  }
}
