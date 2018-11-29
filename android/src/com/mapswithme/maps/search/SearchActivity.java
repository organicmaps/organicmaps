package com.mapswithme.maps.search;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.StyleRes;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.NavUtils;

import com.mapswithme.maps.R;
import com.mapswithme.maps.activity.CustomNavigateUpListener;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.maps.base.OnBackPressListener;
import com.mapswithme.maps.purchase.PurchaseCallback;
import com.mapswithme.maps.purchase.AdsRemovalPurchaseControllerProvider;
import com.mapswithme.maps.purchase.PurchaseController;
import com.mapswithme.maps.purchase.PurchaseFactory;
import com.mapswithme.util.ThemeUtils;

public class SearchActivity extends BaseMwmFragmentActivity
    implements CustomNavigateUpListener, AdsRemovalPurchaseControllerProvider
{
  public static final String EXTRA_QUERY = "search_query";
  public static final String EXTRA_LOCALE = "locale";
  public static final String EXTRA_SEARCH_ON_MAP = "search_on_map";

  @Nullable
  private PurchaseController<PurchaseCallback> mAdsRemovalPurchaseController;

  public static void start(@NonNull Activity activity, @Nullable String query,
                           @Nullable HotelsFilter filter, @Nullable BookingFilterParams params)
  {
    start(activity, query, null /* locale */, false /* isSearchOnMap */,
          filter, params);
  }

  public static void start(@NonNull Activity activity, @Nullable String query, @Nullable String locale,
                           boolean isSearchOnMap, @Nullable HotelsFilter filter,
                           @Nullable BookingFilterParams params)
  {
    final Intent i = new Intent(activity, SearchActivity.class);
    Bundle args = new Bundle();
    args.putString(EXTRA_QUERY, query);
    args.putString(EXTRA_LOCALE, locale);
    args.putBoolean(EXTRA_SEARCH_ON_MAP, isSearchOnMap);
    args.putParcelable(FilterActivity.EXTRA_FILTER, filter);
    args.putParcelable(FilterActivity.EXTRA_FILTER_PARAMS, params);
    i.putExtras(args);
    activity.startActivity(i);
    activity.overridePendingTransition(R.anim.search_fade_in, R.anim.search_fade_out);
  }

  @Override
  protected void safeOnCreate(@Nullable Bundle savedInstanceState)
  {
    super.safeOnCreate(savedInstanceState);
    mAdsRemovalPurchaseController = PurchaseFactory.createAdsRemovalPurchaseController(this);
    mAdsRemovalPurchaseController.initialize(this);
  }

  @Override
  protected void onDestroy()
  {
    super.onDestroy();
    if (mAdsRemovalPurchaseController != null)
      mAdsRemovalPurchaseController.destroy();
  }

  @Override
  @StyleRes
  public int getThemeResourceId(@NonNull String theme)
  {
    return ThemeUtils.getCardBgThemeResourceId(theme);
  }

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return SearchFragment.class;
  }

  @Override
  protected boolean useTransparentStatusBar()
  {
    return false;
  }

  @Override
  protected boolean useColorStatusBar()
  {
    return true;
  }

  @Override
  public void customOnNavigateUp()
  {
    final FragmentManager manager = getSupportFragmentManager();
    if (manager.getBackStackEntryCount() == 0)
    {
      for (Fragment fragment : manager.getFragments())
      {
        if (fragment instanceof HotelsFilterHolder)
        {
          HotelsFilterHolder holder = (HotelsFilterHolder) fragment;
          HotelsFilter filter = holder.getHotelsFilter();
          BookingFilterParams params = holder.getFilterParams();
          if (filter != null || params != null)
          {
            Intent intent = NavUtils.getParentActivityIntent(this);
            intent.putExtra(FilterActivity.EXTRA_FILTER, filter);
            intent.putExtra(FilterActivity.EXTRA_FILTER_PARAMS, params);
            NavUtils.navigateUpTo(this, intent);
            return;
          }
        }
      }
      NavUtils.navigateUpFromSameTask(this);
      overridePendingTransition(android.R.anim.fade_in, android.R.anim.fade_out);
      return;
    }

    manager.popBackStack();
  }

  @Override
  public void onBackPressed()
  {
    for (Fragment f : getSupportFragmentManager().getFragments())
      if ((f instanceof OnBackPressListener) && ((OnBackPressListener)f).onBackPressed())
        return;

    super.onBackPressed();
    overridePendingTransition(android.R.anim.fade_in, android.R.anim.fade_out);
  }

  @Nullable
  @Override
  public PurchaseController<PurchaseCallback> getAdsRemovalPurchaseController()
  {
    return mAdsRemovalPurchaseController;
  }
}
