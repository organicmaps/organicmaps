package com.mapswithme.maps.routing;

import android.content.res.Resources;
import android.util.Pair;

import java.util.ArrayList;
import java.util.List;

import com.mapswithme.maps.LocationState;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;

class ResultCodesHelper
{
  // Codes correspond to native routing::IRouter::ResultCode in routing/router.hpp
  static final int NO_ERROR = 0;
  static final int CANCELLED = 1;
  static final int NO_POSITION = 2;
  private static final int INCONSISTENT_MWM_ROUTE = 3;
  private static final int ROUTING_FILE_NOT_EXIST = 4;
  private static final int START_POINT_NOT_FOUND = 5;
  private static final int END_POINT_NOT_FOUND = 6;
  private static final int DIFFERENT_MWM = 7;
  private static final int ROUTE_NOT_FOUND = 8;
  private static final int NEED_MORE_MAPS = 9;
  private static final int INTERNAL_ERROR = 10;
  private static final int FILE_TOO_OLD = 11;

  static Pair<String, String> getDialogTitleSubtitle(int errorCode, int missingCount)
  {
    Resources resources = MwmApplication.get().getResources();
    int titleRes = 0;
    List<String> messages = new ArrayList<>();
    switch (errorCode)
    {
    case NO_POSITION:
      if (LocationState.INSTANCE.nativeGetMode() == LocationState.NOT_FOLLOW_NO_POSITION)
      {
        titleRes = R.string.dialog_routing_location_turn_on;
        messages.add(resources.getString(R.string.dialog_routing_location_unknown_turn_on));
      }
      else
      {
        titleRes = R.string.dialog_routing_check_gps;
        messages.add(resources.getString(R.string.dialog_routing_error_location_not_found));
        messages.add(resources.getString(R.string.dialog_routing_location_turn_wifi));
      }
      break;
    case INCONSISTENT_MWM_ROUTE:
    case ROUTING_FILE_NOT_EXIST:
      titleRes = R.string.routing_download_maps_along;
      messages.add(resources.getString(R.string.routing_requires_all_map));
      break;
    case START_POINT_NOT_FOUND:
      titleRes = R.string.dialog_routing_change_start;
      messages.add(resources.getString(R.string.dialog_routing_start_not_determined));
      messages.add(resources.getString(R.string.dialog_routing_select_closer_start));
      break;
    case END_POINT_NOT_FOUND:
      titleRes = R.string.dialog_routing_change_end;
      messages.add(resources.getString(R.string.dialog_routing_end_not_determined));
      messages.add(resources.getString(R.string.dialog_routing_select_closer_end));
      break;
    case DIFFERENT_MWM:
      messages.add(resources.getString(R.string.routing_failed_cross_mwm_building));
      break;
    case FILE_TOO_OLD:
      titleRes = R.string.downloader_update_maps;
      messages.add(resources.getString(R.string.downloader_mwm_migration_dialog));
      break;
    case ROUTE_NOT_FOUND:
      if (missingCount == 0)
      {
        titleRes = R.string.dialog_routing_unable_locate_route;
        messages.add(resources.getString(R.string.dialog_routing_cant_build_route));
        messages.add(resources.getString(R.string.dialog_routing_change_start_or_end));
      }
      else
      {
        titleRes = R.string.routing_download_maps_along;
        messages.add(resources.getString(R.string.routing_requires_all_map));
      }
      break;
    case INTERNAL_ERROR:
      titleRes = R.string.dialog_routing_system_error;
      messages.add(resources.getString(R.string.dialog_routing_application_error));
      messages.add(resources.getString(R.string.dialog_routing_try_again));
      break;
    case NEED_MORE_MAPS:
      titleRes = R.string.dialog_routing_download_and_build_cross_route;
      messages.add(resources.getString(R.string.dialog_routing_download_cross_route));
      break;
    }

    StringBuilder builder = new StringBuilder();
    for (String messagePart : messages)
    {
      if (builder.length() > 0)
        builder.append("\n\n");

      builder.append(messagePart);
    }

    return new Pair<>(titleRes == 0 ? "" : resources.getString(titleRes), builder.toString());
  }

  static boolean isDownloadable(int resultCode, int missingCount)
  {
    return (resultCode == INCONSISTENT_MWM_ROUTE ||
            resultCode == ROUTING_FILE_NOT_EXIST ||
            resultCode == NEED_MORE_MAPS ||
            (resultCode == ROUTE_NOT_FOUND && missingCount > 0));
  }
}
