package com.mapswithme.maps.base;

import android.app.Activity;
import android.media.AudioManager;
import android.os.Bundle;
import android.support.annotation.ColorRes;
import android.support.annotation.CallSuper;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.StyleRes;
import android.support.v4.app.Fragment;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.MenuItem;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.util.Config;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;

import static android.Manifest.permission.ACCESS_COARSE_LOCATION;
import static android.Manifest.permission.ACCESS_FINE_LOCATION;
import static android.Manifest.permission.GET_ACCOUNTS;
import static android.Manifest.permission.WRITE_EXTERNAL_STORAGE;
import static android.content.pm.PackageManager.PERMISSION_GRANTED;

public class BaseMwmFragmentActivity extends AppCompatActivity
                                  implements BaseActivity
{
  private static final int REQUEST_PERMISSIONS_START = 1;
  private static final int REQUEST_PERMISSIONS_RESUME = 2;

  private static final String[] START_PERMISSIONS = new String[]
      {
        WRITE_EXTERNAL_STORAGE,
        ACCESS_COARSE_LOCATION,
        ACCESS_FINE_LOCATION,
        GET_ACCOUNTS
      };
  private static final String[] RESUME_PERMISSIONS = new String[] {WRITE_EXTERNAL_STORAGE};

  private final BaseActivityDelegate mBaseDelegate = new BaseActivityDelegate(this);

  @Nullable
  private Bundle mSavedInstanceState;
  private boolean mRequestedFromOnCreate = false;
  private boolean mFromInstanceState = false;

  @Override
  public Activity get()
  {
    return this;
  }

  @Override
  @StyleRes
  public int getThemeResourceId(@NonNull String theme)
  {
    if (ThemeUtils.isDefaultTheme(theme))
        return R.style.MwmTheme;

    if (ThemeUtils.isNightTheme(theme))
      return R.style.MwmTheme_Night;

    throw new IllegalArgumentException("Attempt to apply unsupported theme: " + theme);
  }

  @Override
  protected void onCreate(@Nullable Bundle savedInstanceState)
  {
    mFromInstanceState = savedInstanceState != null;
    if (!Utils.checkPermissions(this, MwmApplication.get().isPlatformInitialized()
                                      ? RESUME_PERMISSIONS : START_PERMISSIONS,
                                REQUEST_PERMISSIONS_START))
    {
      mRequestedFromOnCreate = true;
      super.onCreate(savedInstanceState);
      return;
    }

    MwmApplication.get().initNativePlatform();
    mBaseDelegate.onCreate();

    super.onCreate(savedInstanceState);
    safeOnCreate(savedInstanceState);
  }

  @CallSuper
  protected void safeOnCreate(@Nullable Bundle savedInstanceState)
  {
    setVolumeControlStream(AudioManager.STREAM_MUSIC);
    final int layoutId = getContentLayoutResId();
    if (layoutId != 0)
      setContentView(layoutId);

    if (useTransparentStatusBar())
      UiUtils.setupStatusBar(this);
    if (useColorStatusBar())
      UiUtils.setupColorStatusBar(this, getStatusBarColor());

    // Use full-screen on Kindle Fire only
    if (Utils.isAmazonDevice())
    {
      getWindow().addFlags(android.view.WindowManager.LayoutParams.FLAG_FULLSCREEN);
      getWindow().clearFlags(android.view.WindowManager.LayoutParams.FLAG_FORCE_NOT_FULLSCREEN);
    }

    MwmApplication.get().initNativeCore();
    MwmApplication.get().initCounters();

    LocationHelper.INSTANCE.init();

    attachDefaultFragment();
  }

  @ColorRes
  protected int getStatusBarColor()
  {
    String theme = Config.getCurrentUiTheme();
    if (ThemeUtils.isDefaultTheme(theme))
      return R.color.bg_statusbar;

    if (ThemeUtils.isNightTheme(theme))
      return R.color.bg_statusbar_night;

    throw new IllegalArgumentException("Attempt to apply unsupported theme: " + theme);
  }

  protected boolean useColorStatusBar()
  {
    return false;
  }

  protected boolean useTransparentStatusBar()
  {
    return true;
  }

  @Override
  protected void onPostCreate(@Nullable Bundle savedInstanceState)
  {
    super.onPostCreate(savedInstanceState);
    mBaseDelegate.onPostCreate();
  }

  @Override
  protected void onDestroy()
  {
    super.onDestroy();
    mBaseDelegate.onDestroy();
  }

  @Override
  protected void onStart()
  {
    super.onStart();
    mBaseDelegate.onStart();
    if (MwmApplication.get().isPlatformInitialized())
      safeOnStart();
  }

  protected void safeOnStart()
  {
  }

  @Override
  protected void onStop()
  {
    super.onStop();
    mBaseDelegate.onStop();
  }

  @Override
  protected void onRestoreInstanceState(Bundle savedInstanceState)
  {
    super.onRestoreInstanceState(savedInstanceState);
    mSavedInstanceState = savedInstanceState;
    if (MwmApplication.get().isPlatformInitialized() && Utils.isWriteExternalGranted(this))
      safeOnRestoreInstanceState(savedInstanceState);
  }

  protected void safeOnRestoreInstanceState(@NonNull Bundle savedInstanceState)
  {
  }

  @Nullable
  public Bundle getSavedInstanceState()
  {
    return mSavedInstanceState;
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item)
  {
    if (item.getItemId() == android.R.id.home)
    {
      onBackPressed();
      return true;
    }
    return super.onOptionsItemSelected(item);
  }

  @Override
  protected void onResume()
  {
    super.onResume();
    if (!mRequestedFromOnCreate && !Utils.checkPermissions(this, RESUME_PERMISSIONS,
                                                           REQUEST_PERMISSIONS_RESUME))
    {
      return;
    }

    mBaseDelegate.onResume();
    if (MwmApplication.get().isPlatformInitialized() && Utils.isWriteExternalGranted(this))
      safeOnResume();
  }

  protected void safeOnResume()
  {
  }

  @Override
  protected void onPostResume()
  {
    super.onPostResume();
    mBaseDelegate.onPostResume();
  }

  @CallSuper
  @Override
  protected void onResumeFragments()
  {
    super.onResumeFragments();
    if (MwmApplication.get().isPlatformInitialized() && Utils.isWriteExternalGranted(this))
      safeOnResumeFragments();
  }

  protected void safeOnResumeFragments()
  {
  }

  @Override
  protected void onPause()
  {
    super.onPause();
    mBaseDelegate.onPause();
  }

  @Override
  public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults)
  {
    super.onRequestPermissionsResult(requestCode, permissions, grantResults);
    if (grantResults.length == 0)
      return;

    boolean isWriteGranted = false;
    for (int i = 0; i < permissions.length; ++i)
    {
      int result = grantResults[i];
      String permission = permissions[i];
      if (permission.equals(WRITE_EXTERNAL_STORAGE) && result == PERMISSION_GRANTED)
        isWriteGranted = true;
    }
    if (isWriteGranted)
    {
      if (requestCode == REQUEST_PERMISSIONS_START)
      {
        MwmApplication.get().initNativePlatform();
        mBaseDelegate.onCreate();
        safeOnCreate(mSavedInstanceState);
        mBaseDelegate.onStart();
        if (mSavedInstanceState != null)
          safeOnRestoreInstanceState(mSavedInstanceState);
        mBaseDelegate.onResume();
        safeOnResume();
        if (!mFromInstanceState)
          safeOnResumeFragments();
      }
      else if (requestCode == REQUEST_PERMISSIONS_RESUME)
      {
        safeOnResume();
        safeOnResumeFragments();
      }
    }
    else
    {
      finish();
    }
  }

  protected Toolbar getToolbar()
  {
    return (Toolbar) findViewById(R.id.toolbar);
  }

  protected void displayToolbarAsActionBar()
  {
    setSupportActionBar(getToolbar());
  }

  /**
   * Override to set custom content view.
   * @return layout resId.
   */
  protected int getContentLayoutResId()
  {
    return 0;
  }

  protected void attachDefaultFragment()
  {
    Class<? extends Fragment> clazz = getFragmentClass();
    if (clazz != null)
      replaceFragment(clazz, getIntent().getExtras(), null);
  }

  /**
   * Replace attached fragment with the new one.
   */
  public void replaceFragment(@NonNull Class<? extends Fragment> fragmentClass, @Nullable Bundle args, @Nullable Runnable completionListener)
  {
    final int resId = getFragmentContentResId();
    if (resId <= 0 || findViewById(resId) == null)
      throw new IllegalStateException("Fragment can't be added, since getFragmentContentResId() isn't implemented or returns wrong resourceId.");

    String name = fragmentClass.getName();
    final Fragment fragment = Fragment.instantiate(this, name, args);
    getSupportFragmentManager().beginTransaction()
                               .replace(resId, fragment, name)
                               .commitAllowingStateLoss();
    getSupportFragmentManager().executePendingTransactions();

    if (completionListener != null)
      completionListener.run();
  }

  /**
   * Override to automatically attach fragment in onCreate. Tag applied to fragment in back stack is set to fragment name, too.
   * WARNING : if custom layout for activity is set, getFragmentContentResId() must be implemented, too.
   * @return class of the fragment, eg FragmentClass.getClass()
   */
  protected Class<? extends Fragment> getFragmentClass()
  {
    return null;
  }

  /**
   * Get resource id for the fragment. That must be implemented to return correct resource id, if custom layout is set.
   * @return resourceId for the fragment
   */
  protected int getFragmentContentResId()
  {
    return android.R.id.content;
  }
}
