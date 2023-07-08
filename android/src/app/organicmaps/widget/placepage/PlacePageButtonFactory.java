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
    @DrawableRes int iconId = 0;
    switch (buttonType)
    {
      case BACK:
        titleId = R.string.back;
        iconId = ThemeUtils.getResource(MwmApplication.from(context), android.R.attr.homeAsUpIndicator);
        break;
      case BOOKMARK_SAVE:
        titleId = R.string.save;
        iconId = R.drawable.ic_bookmarks_off;
        break;
      case BOOKMARK_DELETE:
        titleId = R.string.delete;
        iconId = R.drawable.ic_bookmarks_on;
        break;
      case ROUTE_FROM:
        titleId = R.string.p2p_from_here;
        iconId = R.drawable.ic_route_from;
        break;
      case ROUTE_TO:
        titleId = R.string.p2p_to_here;
        iconId = R.drawable.ic_route_to;
        break;
      case ROUTE_ADD:
        titleId = R.string.placepage_add_stop;
        iconId = R.drawable.ic_route_via;
        break;
      case ROUTE_REMOVE:
        titleId = R.string.placepage_remove_stop;
        iconId = R.drawable.ic_route_remove;
        break;
      case ROUTE_AVOID_TOLL:
        titleId = R.string.avoid_toll_roads_placepage;
        iconId = R.drawable.ic_avoid_tolls;
        break;
      case ROUTE_AVOID_UNPAVED:
        titleId = R.string.avoid_unpaved_roads_placepage;
        iconId = R.drawable.ic_avoid_unpaved;
        break;
      case ROUTE_AVOID_FERRY:
        titleId = R.string.avoid_ferry_crossing_placepage;
        iconId = R.drawable.ic_avoid_ferry;
        break;
      case MORE:
        titleId = R.string.placepage_more_button;
        iconId = R.drawable.ic_more;
        break;
    }
    return new PlacePageButton(titleId, iconId, buttonType);
  }
}
