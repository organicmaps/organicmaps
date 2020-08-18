package com.mapswithme.maps.widget.menu;

import androidx.annotation.NonNull;
import com.mapswithme.maps.search.FilterUtils;

public interface MenuRoomsGuestsListener
{
  void onRoomsGuestsApplied(@NonNull FilterUtils.RoomGuestCounts counts);
}
