package com.mapswithme.maps.data;

import android.util.Pair;

import com.mapswithme.maps.MWMApplication;
import com.mapswithme.maps.R;

// Codes correspond to native routing::IRouter::ResultCode
public class RoutingResultCodes
{
  public static final int NO_ERROR = 0;
  public static final int CANCELLED = 1;
  public static final int NO_POSITION = 2;
  public static final int INCONSISTENT_MWM_ROUTE = 3;
  public static final int ROUTING_FILE_NOT_EXIST = 4;
  public static final int START_POINT_NOT_FOUND = 5;
  public static final int END_POINT_NOT_FOUND = 6;
  public static final int DIFFERENT_MWM = 7;
  public static final int ROUTE_NOT_FOUND = 8;
  public static final int INTERNAL_ERROR = 9;

  public static Pair<String, String> getDialogTitleSubtitle(int errorCode)
  {
    int titleRes = 0, messageRes = 0;
    switch (errorCode)
    {
    case NO_POSITION:
      messageRes = R.string.routing_failed_unknown_my_position;
      break;
    case INCONSISTENT_MWM_ROUTE:
    case ROUTING_FILE_NOT_EXIST:
      // FIXME use correct translated title
      titleRes = R.string.get_it_now;
      messageRes = R.string.routing_failed_has_no_routing_file;
      break;
    case START_POINT_NOT_FOUND:
      messageRes = R.string.routing_failed_start_point_not_found;
      break;
    case END_POINT_NOT_FOUND:
      messageRes = R.string.routing_failed_dst_point_not_found;
      break;
    case DIFFERENT_MWM:
      messageRes = R.string.routing_failed_cross_mwm_building;
      break;
    case ROUTE_NOT_FOUND:
      messageRes = R.string.routing_failed_route_not_found;
      break;
    case INTERNAL_ERROR:
      messageRes = R.string.routing_failed_internal_error;
      break;
    }

    return new Pair<>(titleRes == 0 ? "" : MWMApplication.get().getString(titleRes), MWMApplication.get().getString(messageRes));
  }
}
