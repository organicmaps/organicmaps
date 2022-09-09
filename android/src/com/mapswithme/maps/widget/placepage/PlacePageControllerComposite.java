package com.mapswithme.maps.widget.placepage;

import android.app.Activity;
import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.util.bottomsheet.MenuBottomSheetItem;

import java.util.ArrayList;
import java.util.List;

class PlacePageControllerComposite implements PlacePageController
{
  @NonNull
  private final PlacePageController.SlideListener mSlideListener;
  @Nullable
  private final RoutingModeListener mRoutingModeListener;
  @NonNull
  private final List<PlacePageController> mControllers = new ArrayList<>();
  @SuppressWarnings("NullableProblems")
  @NonNull
  private PlacePageController mActiveController;

  PlacePageControllerComposite(@NonNull SlideListener slideListener,
                               @Nullable RoutingModeListener routingModeListener)
  {
    mSlideListener = slideListener;
    mRoutingModeListener = routingModeListener;
  }

  @Override
  public int getPlacePageWidth()
  {
    return mActiveController.getPlacePageWidth();
  }

  @Override
  @Nullable
  public ArrayList<MenuBottomSheetItem> getMenuBottomSheetItems()
  {
    return mActiveController.getMenuBottomSheetItems();
  }

  @Override
  public void openFor(@NonNull PlacePageData data)
  {
    boolean support = mActiveController.support(data);
    if (support)
    {
      mActiveController.openFor(data);
      return;
    }

    mActiveController.close(false);
    PlacePageController controller = findControllerFor(data);
    if (controller == null)
      throw new UnsupportedOperationException("Place page data '" + data + "' not supported " +
                                              "by existing controllers");
    mActiveController = controller;
    mActiveController.openFor(data);
  }

  @Override
  public void close(boolean deactivateMapSelection)
  {
    mActiveController.close(deactivateMapSelection);
  }

  @Override
  public boolean isClosed()
  {
    return mActiveController.isClosed();
  }

  @Override
  public void onActivityCreated(Activity activity, Bundle savedInstanceState)
  {
    mActiveController.onActivityCreated(activity, savedInstanceState);
  }

  @Override
  public void onActivityStarted(Activity activity)
  {
    mActiveController.onActivityStarted(activity);
  }

  @Override
  public void onActivityResumed(Activity activity)
  {
    mActiveController.onActivityResumed(activity);
  }

  @Override
  public void onActivityPaused(Activity activity)
  {
    mActiveController.onActivityPaused(activity);
  }

  @Override
  public void onActivityStopped(Activity activity)
  {
    mActiveController.onActivityStopped(activity);
  }

  @Override
  public void onActivitySaveInstanceState(Activity activity, Bundle outState)
  {
    mActiveController.onActivitySaveInstanceState(activity, outState);
  }

  @Override
  public void onActivityDestroyed(Activity activity)
  {
    mActiveController.onActivityDestroyed(activity);
  }

  @Override
  public void initialize(@Nullable Activity activity)
  {
    if (!mControllers.isEmpty())
      throw new AssertionError("Place page controllers already initialized!");

    PlacePageController richController =
        PlacePageFactory.createRichController(mSlideListener, mRoutingModeListener);
    richController.initialize(activity);
    mControllers.add(richController);

    PlacePageController elevationProfileController =
        PlacePageFactory.createElevationProfilePlacePageController(mSlideListener);
    elevationProfileController.initialize(activity);
    mControllers.add(elevationProfileController);

    mActiveController = richController;
  }


  @Override
  public void destroy()
  {
    if (mControllers.isEmpty())
      throw new AssertionError("Place page controllers already destroyed!");

    for (PlacePageController controller: mControllers)
      controller.destroy();

    mControllers.clear();
  }

  @Override
  public void onSave(@NonNull Bundle outState)
  {
    mActiveController.onSave(outState);
  }

  @Override
  public void onRestore(@NonNull Bundle inState)
  {
    PlacePageData data = inState.getParcelable(PlacePageUtils.EXTRA_PLACE_PAGE_DATA);
    if (data != null)
    {
      PlacePageController controller = findControllerFor(data);
      if (controller != null)
        mActiveController = controller;
    }
    mActiveController.onRestore(inState);
  }

  @Nullable
  private PlacePageController findControllerFor(@NonNull PlacePageData object)
  {
    for (PlacePageController controller : mControllers)
    {
      if (controller.support(object))
        return controller;
    }

    return null;
  }

  @Override
  public boolean support(@NonNull PlacePageData object)
  {
    return mActiveController.support(object);
  }
}
