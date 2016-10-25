package com.mapswithme.maps.search;

import android.support.annotation.NonNull;
import android.text.TextUtils;
import android.util.Pair;
import com.mapswithme.util.Language;

import java.util.ArrayList;
import java.util.List;

public final class SearchRecents
{
  private static final List<String> sRecents = new ArrayList<>();

  private SearchRecents() {}

  public static void refresh()
  {
    final List<Pair<String, String>> pairs = new ArrayList<>();
    nativeGetList(pairs);
    sRecents.clear();

    for (Pair<String, String> pair : pairs)
      sRecents.add(pair.second);
  }

  public static int getSize()
  {
    return sRecents.size();
  }

  public static String get(int position)
  {
    return sRecents.get(position);
  }

  public static boolean add(@NonNull String query)
  {
    if (TextUtils.isEmpty(query) || sRecents.contains(query))
      return false;

    nativeAdd(Language.getKeyboardLocale(), query);
    refresh();
    return true;
  }

  public static void clear()
  {
    nativeClear();
    sRecents.clear();
  }

  private static native void nativeGetList(List<Pair<String, String>> result);
  private static native void nativeAdd(String locale, String query);
  private static native void nativeClear();
}
