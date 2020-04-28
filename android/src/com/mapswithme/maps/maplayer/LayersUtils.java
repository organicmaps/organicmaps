package com.mapswithme.maps.maplayer;

import android.content.Context;
import android.util.Pair;

import androidx.annotation.NonNull;
import com.mapswithme.maps.adapter.OnItemClickListener;

import java.util.Arrays;
import java.util.List;

public class LayersUtils
{
  @NonNull
  public static List<Pair<BottomSheetItem, OnItemClickListener<BottomSheetItem>>> createItems(
      @NonNull Context context, @NonNull OnItemClickListener<BottomSheetItem> subwayListener,
      @NonNull OnItemClickListener<BottomSheetItem> trafficListener,
      @NonNull OnItemClickListener<BottomSheetItem> isoLinesListener,
      @NonNull OnItemClickListener<BottomSheetItem> guidesListener)
  {
    Pair<BottomSheetItem, OnItemClickListener<BottomSheetItem>> subway
        = new Pair<>(BottomSheetItem.Subway.makeInstance(context), subwayListener);

    Pair<BottomSheetItem, OnItemClickListener<BottomSheetItem>> traffic
        = new Pair<>(BottomSheetItem.Traffic.makeInstance(context), trafficListener);

    Pair<BottomSheetItem, OnItemClickListener<BottomSheetItem>> isoLines
        = new Pair<>(BottomSheetItem.Isolines.makeInstance(context), isoLinesListener);

    Pair<BottomSheetItem, OnItemClickListener<BottomSheetItem>> guides
        = new Pair<>(BottomSheetItem.Guides.makeInstance(context), guidesListener);

    return Arrays.asList(guides, traffic, isoLines, subway);
  }
}
