package app.organicmaps.maplayer;

import android.view.View;

import androidx.annotation.NonNull;

import app.organicmaps.adapter.OnItemClickListener;

public class RouteBottomSheetItem
{
  @NonNull
  private final String mRouteName;
  @NonNull
  private final OnItemClickListener<RouteBottomSheetItem> mItemTitleClickListener;
  @NonNull
  private final OnItemClickListener<RouteBottomSheetItem> mItemRenameClickListener;
  @NonNull
  private final OnItemClickListener<RouteBottomSheetItem> mItemDeleteClickListener;

  RouteBottomSheetItem(@NonNull String routeName,
                       @NonNull OnItemClickListener<RouteBottomSheetItem> itemTitleClickListener,
                       @NonNull OnItemClickListener<RouteBottomSheetItem> itemRenameClickListener,
                       @NonNull OnItemClickListener<RouteBottomSheetItem> itemDeleteClickListener)
  {
    mRouteName = routeName;
    mItemTitleClickListener = itemTitleClickListener;
    mItemRenameClickListener = itemRenameClickListener;
    mItemDeleteClickListener = itemDeleteClickListener;
  }

  public static RouteBottomSheetItem create(@NonNull String routeName,
                                            @NonNull OnItemClickListener<RouteBottomSheetItem> routeItemTitleClickListener,
                                            @NonNull OnItemClickListener<RouteBottomSheetItem> routeItemRenameClickListener,
                                            @NonNull OnItemClickListener<RouteBottomSheetItem> routeItemDeleteClickListener)
  {
    return new RouteBottomSheetItem(routeName, routeItemTitleClickListener, routeItemRenameClickListener, routeItemDeleteClickListener);
  }

  public String getRouteName()
  {
    return mRouteName;
  }

  public void onTitleClick(@NonNull View v, @NonNull RouteBottomSheetItem item)
  {
    mItemTitleClickListener.onItemClick(v, item);
  }

  public void onRenameClick(@NonNull View v, @NonNull RouteBottomSheetItem item)
  {
    mItemRenameClickListener.onItemClick(v, item);
  }

  public void onDeleteClick(@NonNull View v, @NonNull RouteBottomSheetItem item)
  {
    mItemDeleteClickListener.onItemClick(v, item);
  }
}
