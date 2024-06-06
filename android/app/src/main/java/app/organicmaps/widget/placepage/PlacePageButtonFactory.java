package app.organicmaps.widget.placepage;

import android.content.Context;

import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.StringRes;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.util.ThemeUtils;

public class PlacePageButtonFactory
{
  static PlacePageButton createButton(PlacePageButtons.ButtonType buttonType, @NonNull Context context)
  {
    @StringRes int titleId = 0;
    @DrawableRes int iconId = switch (buttonType)
    {
      case BACK ->
      {
        titleId = R.string.back;
        yield ThemeUtils.getResource(MwmApplication.from(context), android.R.attr.homeAsUpIndicator);
      }
      case BOOKMARK_SAVE ->
      {
        titleId = nativeHasRecentlyDeletedBookmark() ? R.string.restore : R.string.save;
        yield R.drawable.ic_bookmarks_off;
      }
      case BOOKMARK_DELETE ->
      {
        titleId = R.string.delete;
        yield R.drawable.ic_bookmarks_on;
      }
      case ROUTE_FROM ->
      {
        titleId = R.string.p2p_from_here;
        yield R.drawable.ic_route_from;
      }
      case ROUTE_TO ->
      {
        titleId = R.string.p2p_to_here;
        yield R.drawable.ic_route_to;
      }
      case ROUTE_ADD ->
      {
        titleId = R.string.placepage_add_stop;
        yield R.drawable.ic_route_via;
      }
      case ROUTE_REMOVE ->
      {
        titleId = R.string.placepage_remove_stop;
        yield R.drawable.ic_route_remove;
      }
      case ROUTE_AVOID_TOLL ->
      {
        titleId = R.string.avoid_tolls;
        yield R.drawable.ic_avoid_tolls;
      }
      case ROUTE_AVOID_UNPAVED ->
      {
        titleId = R.string.avoid_unpaved;
        yield R.drawable.ic_avoid_unpaved;
      }
      case ROUTE_AVOID_FERRY ->
      {
        titleId = R.string.avoid_ferry;
        yield R.drawable.ic_avoid_ferry;
      }
      case MORE ->
      {
        titleId = R.string.placepage_more_button;
        yield R.drawable.ic_more;
      }
    };
    return new PlacePageButton(titleId, iconId, buttonType);
  }

  private native static boolean nativeHasRecentlyDeletedBookmark();
}
