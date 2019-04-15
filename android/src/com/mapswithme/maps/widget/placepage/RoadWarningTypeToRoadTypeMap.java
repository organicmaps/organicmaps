package com.mapswithme.maps.widget.placepage;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.bookmarks.data.RoadWarningMarkType;
import com.mapswithme.maps.widget.placepage.PlacePageButtons;

import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

public class RoadWarningTypeToRoadTypeMap
{
  private final static Map<RoadWarningMarkType, PlacePageButtons.Item> sMap = Collections.unmodifiableMap(createEntries());

  @Nullable
  public static PlacePageButtons.Item get(@NonNull RoadWarningMarkType key)
  {
    return sMap.get(key);
  }

  @NonNull
  private static Map<RoadWarningMarkType, PlacePageButtons.Item> createEntries()
  {
    return new HashMap<RoadWarningMarkType, PlacePageButtons.Item>(){
      private static final long serialVersionUID = -5608368159273229727L;
      {
        put(RoadWarningMarkType.DIRTY, PlacePageButtons.Item.ROUTE_AVOID_UNPAVED);
        put(RoadWarningMarkType.FERRY, PlacePageButtons.Item.ROUTE_AVOID_FERRY);
        put(RoadWarningMarkType.TOLL, PlacePageButtons.Item.ROUTE_AVOID_TOLL);
      }
    };
  }
}
