package com.mapswithme.maps.search;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;

import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StyleRes;
import androidx.core.app.NavUtils;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.maps.base.CustomNavigateUpListener;
import com.mapswithme.maps.base.OnBackPressListener;
import com.mapswithme.maps.dialog.AlertDialogCallback;
import com.mapswithme.maps.purchase.AdsRemovalPurchaseControllerProvider;
import com.mapswithme.maps.purchase.PurchaseCallback;
import com.mapswithme.maps.purchase.PurchaseController;
import com.mapswithme.maps.purchase.PurchaseFactory;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.Utils;

public class SearchActivity extends BaseMwmFragmentActivity
    implements CustomNavigateUpListener, AdsRemovalPurchaseControllerProvider,
               AlertDialogCallback
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
    args.putParcelable(FilterUtils.EXTRA_FILTER_PARAMS, params);
    i.putExtras(args);
    activity.startActivity(i);
    activity.overridePendingTransition(R.anim.search_fade_in, R.anim.search_fade_out);
  }

  @Override
  protected void onSafeCreate(@Nullable Bundle savedInstanceState)
  {
    super.onSafeCreate(savedInstanceState);
    mAdsRemovalPurchaseController = PurchaseFactory.createAdsRemovalPurchaseController(this);
    mAdsRemovalPurchaseController.initialize(this);
  }

  @CallSuper
  @Override
  protected void onSafeDestroy()
  {
    super.onSafeDestroy();
    if (mAdsRemovalPurchaseController != null)
      mAdsRemovalPurchaseController.destroy();
  }

  @Override
  @StyleRes
  public int getThemeResourceId(@NonNull String theme)
  {
    return ThemeUtils.getCardBgThemeResourceId(getApplicationContext(), theme);
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
            intent.putExtra(FilterUtils.EXTRA_FILTER_PARAMS, params);
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

  @Override
  public void onAlertDialogPositiveClick(int requestCode, int which)
  {
    if (requestCode == FilterUtils.REQ_CODE_NO_NETWORK_CONNECTION_DIALOG)
      Utils.showSystemConnectionSettings(this);
  }

  @Override
  public void onAlertDialogNegativeClick(int requestCode, int which)
  {
    // No op.
  }

  @Override
  public void onAlertDialogCancel(int requestCode)
  {
    // No op.
  }
}
