package app.organicmaps.routing;

import android.content.Context;
import android.content.res.Resources;
import android.util.Pair;
import androidx.annotation.NonNull;
import androidx.annotation.StringRes;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.sdk.routing.ResultCodes;
import java.util.ArrayList;
import java.util.List;

public class ResultCodesHelper
{
  @NonNull
  public static ResourcesHolder getDialogTitleSubtitle(@NonNull Context context, int errorCode, int missingCount)
  {
    Resources resources = context.getResources();
    int titleRes = 0;
    List<String> messages = new ArrayList<>();
    @StringRes
    int cancelBtnResId = android.R.string.cancel;
    switch (errorCode)
    {
    case ResultCodes.NO_POSITION:
      if (!MwmApplication.from(context).getLocationHelper().isActive())
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
    case ResultCodes.INCONSISTENT_MWM_ROUTE:
    case ResultCodes.ROUTING_FILE_NOT_EXIST:
      titleRes = R.string.routing_download_maps_along;
      messages.add(resources.getString(R.string.routing_requires_all_map));
      break;
    case ResultCodes.START_POINT_NOT_FOUND:
      titleRes = R.string.dialog_routing_change_start;
      messages.add(resources.getString(R.string.dialog_routing_start_not_determined));
      messages.add(resources.getString(R.string.dialog_routing_select_closer_start));
      break;
    case ResultCodes.END_POINT_NOT_FOUND:
      titleRes = R.string.dialog_routing_change_end;
      messages.add(resources.getString(R.string.dialog_routing_end_not_determined));
      messages.add(resources.getString(R.string.dialog_routing_select_closer_end));
      break;
    case ResultCodes.INTERMEDIATE_POINT_NOT_FOUND:
      titleRes = R.string.dialog_routing_change_intermediate;
      messages.add(resources.getString(R.string.dialog_routing_intermediate_not_determined));
      break;
    case ResultCodes.DIFFERENT_MWM:
      messages.add(resources.getString(R.string.routing_failed_cross_mwm_building));
      break;
    case ResultCodes.FILE_TOO_OLD:
      titleRes = R.string.downloader_update_maps;
      messages.add(resources.getString(R.string.downloader_mwm_migration_dialog));
      break;
    case ResultCodes.TRANSIT_ROUTE_NOT_FOUND_NO_NETWORK:
      messages.add(resources.getString(R.string.transit_not_found));
      break;
    case ResultCodes.TRANSIT_ROUTE_NOT_FOUND_TOO_LONG_PEDESTRIAN:
      titleRes = R.string.dialog_pedestrian_route_is_long_header;
      messages.add(resources.getString(R.string.dialog_pedestrian_route_is_long_message));
      cancelBtnResId = R.string.ok;
      break;
    case ResultCodes.ROUTE_NOT_FOUND:
    case ResultCodes.ROUTE_NOT_FOUND_REDRESS_ROUTE_ERROR:
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
    case ResultCodes.INTERNAL_ERROR:
      titleRes = R.string.dialog_routing_system_error;
      messages.add(resources.getString(R.string.dialog_routing_application_error));
      messages.add(resources.getString(R.string.dialog_routing_try_again));
      break;
    case ResultCodes.NEED_MORE_MAPS:
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

    return new ResourcesHolder(new Pair<>(titleRes == 0 ? "" : resources.getString(titleRes), builder.toString()),
                               cancelBtnResId);
  }

  public static boolean isDownloadable(int resultCode, int missingCount)
  {
    if (missingCount <= 0)
      return false;

    return switch (resultCode)
    {
      case ResultCodes.INCONSISTENT_MWM_ROUTE, ResultCodes.ROUTE_NOT_FOUND_REDRESS_ROUTE_ERROR,
          ResultCodes.ROUTING_FILE_NOT_EXIST, ResultCodes.NEED_MORE_MAPS, ResultCodes.ROUTE_NOT_FOUND,
          ResultCodes.FILE_TOO_OLD ->
        true;
      default -> false;
    };
  }

  public static boolean isMoreMapsNeeded(int resultCode)
  {
    return resultCode == ResultCodes.NEED_MORE_MAPS;
  }

  public static class ResourcesHolder
  {
    @NonNull
    private final Pair<String, String> mTitleMessage;

    private final int mCancelBtnResId;

    private ResourcesHolder(@NonNull Pair<String, String> titleMessage, int cancelBtnResId)
    {
      mTitleMessage = titleMessage;
      mCancelBtnResId = cancelBtnResId;
    }

    @NonNull
    public Pair<String, String> getTitleMessage()
    {
      return mTitleMessage;
    }

    public int getCancelBtnResId()
    {
      return mCancelBtnResId;
    }
  }
}
