package com.mapswithme.maps.base;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.media.AudioManager;
import android.os.Bundle;
import androidx.annotation.CallSuper;
import androidx.annotation.ColorRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StyleRes;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import android.view.MenuItem;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.SplashActivity;
import com.mapswithme.util.Config;
import com.mapswithme.util.PermissionsUtils;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

public abstract class BaseMwmFragmentActivity extends AppCompatActivity
                                  implements BaseActivity
{
  private final BaseActivityDelegate mBaseDelegate = new BaseActivityDelegate(this);
  private boolean mSafeCreated;

  @Override
  @NonNull
  public Activity get()
  {
    return this;
  }

  @Override
  @StyleRes
  public int getThemeResourceId(@NonNull String theme)
  {
    Context context = getApplicationContext();

    if (ThemeUtils.isDefaultTheme(context, theme))
        return R.style.MwmTheme;

    if (ThemeUtils.isNightTheme(context, theme))
      return R.style.MwmTheme_Night;

    throw new IllegalArgumentException("Attempt to apply unsupported theme: " + theme);
  }

  /**
   * Shows splash screen and initializes the core in case when it was not initialized.
   *
   * Do not override this method!
   * Use {@link #onSafeCreate(Bundle savedInstanceState)}
   */
  @CallSuper
  @Override
  protected final void onCreate(@Nullable Bundle savedInstanceState)
  {
    mBaseDelegate.onCreate();
    // An intent that was skipped due to core wasn't initialized has to be used
    // as a target intent for this activity, otherwise all input extras will be lost
    // in a splash activity loop.
    Intent initialIntent = getIntent().getParcelableExtra(SplashActivity.EXTRA_INITIAL_INTENT);
    if (initialIntent != null)
      setIntent(initialIntent);

    if (!MwmApplication.from(this).arePlatformAndCoreInitialized()
        || !PermissionsUtils.isExternalStorageGranted(getApplicationContext()))
    {
      super.onCreate(savedInstanceState);
      goToSplashScreen(getIntent());
      return;
    }

    super.onCreate(savedInstanceState);

    onSafeCreate(savedInstanceState);
  }

  /**
   * Use this safe method instead of {@link #onCreate(Bundle savedInstanceState)}.
   * When this method is called, the core is already initialized.
   */
  @CallSuper
  protected void onSafeCreate(@Nullable Bundle savedInstanceState)
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

    attachDefaultFragment();
    mBaseDelegate.onSafeCreate();
    mSafeCreated = true;
  }

  @ColorRes
  protected int getStatusBarColor()
  {
    Context context = getApplicationContext();
    if (ThemeUtils.isDefaultTheme(context))
      return R.color.bg_statusbar;

    if (ThemeUtils.isNightTheme(context))
      return R.color.bg_statusbar_night;

    throw new IllegalArgumentException("Attempt to apply unsupported theme: "
                                       + Config.getCurrentUiTheme(context));
  }

  protected boolean useColorStatusBar()
  {
    return false;
  }

  protected boolean useTransparentStatusBar()
  {
    return true;
  }

  @CallSuper
  @Override
  protected void onNewIntent(Intent intent)
  {
    super.onNewIntent(intent);
    mBaseDelegate.onNewIntent(intent);
  }

  @CallSuper
  @Override
  protected void onPostCreate(@Nullable Bundle savedInstanceState)
  {
    super.onPostCreate(savedInstanceState);
    mBaseDelegate.onPostCreate();
  }

  @CallSuper
  @Override
  protected final void onDestroy()
  {
    super.onDestroy();
    mBaseDelegate.onDestroy();

    if (!mSafeCreated)
      return;

    onSafeDestroy();
  }

  /**
   * Use this safe method instead of {@link #onDestroy()}.
   * When this method is called, the core is already initialized and
   * {@link #onSafeCreate(Bundle savedInstanceState)} was called.
   */
  @CallSuper
  protected void onSafeDestroy()
  {
    mBaseDelegate.onSafeDestroy();
    mSafeCreated = false;
  }

  @CallSuper
  @Override
  protected void onStart()
  {
    super.onStart();
    mBaseDelegate.onStart();
  }

  @CallSuper
  @Override
  protected void onStop()
  {
    super.onStop();
    mBaseDelegate.onStop();
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item)
  {
    if (item.getItemId() == android.R.id.home)
    {
      onHomeOptionItemSelected();
      return true;
    }
    return super.onOptionsItemSelected(item);
  }

  protected void onHomeOptionItemSelected()
  {
    onBackPressed();
  }

  @CallSuper
  @Override
  protected void onResume()
  {
    super.onResume();
    if (!PermissionsUtils.isExternalStorageGranted(getApplicationContext()))
    {
      goToSplashScreen(null);
      return;
    }

    mBaseDelegate.onResume();
  }

  @CallSuper
  @Override
  protected void onPostResume()
  {
    super.onPostResume();
    mBaseDelegate.onPostResume();
  }

  @CallSuper
  @Override
  protected void onPause()
  {
    super.onPause();
    mBaseDelegate.onPause();
  }

  protected Toolbar getToolbar()
  {
    return (Toolbar) findViewById(R.id.toolbar);
  }

  protected void displayToolbarAsActionBar()
  {
    setSupportActionBar(getToolbar());
  }

  @Override
  public void onBackPressed()
  {
    if (getFragmentClass() == null)
    {
      super.onBackPressed();
      return;
    }
    FragmentManager manager = getSupportFragmentManager();
    String name = getFragmentClass().getName();
    Fragment fragment = manager.findFragmentByTag(name);

    if (fragment == null)
    {
      super.onBackPressed();
      return;
    }

    if (onBackPressedInternal(fragment))
      return;

    super.onBackPressed();
  }

  private boolean onBackPressedInternal(@NonNull Fragment currentFragment)
  {
    try
    {
      OnBackPressListener listener = (OnBackPressListener) currentFragment;
      return listener.onBackPressed();
    }
    catch (ClassCastException e)
    {
      Logger logger = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
      String tag = this.getClass().getSimpleName();
      logger.i(tag, "Fragment '" + currentFragment + "' doesn't handle back press by itself.");
      return false;
    }
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
    Fragment potentialInstance = getSupportFragmentManager().findFragmentByTag(name);
    if (potentialInstance == null)
    {
      final Fragment fragment = Fragment.instantiate(this, name, args);
      getSupportFragmentManager().beginTransaction()
                                 .replace(resId, fragment, name)
                                 .commitAllowingStateLoss();
      getSupportFragmentManager().executePendingTransactions();
      if (completionListener != null)
        completionListener.run();
    }
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

  private void goToSplashScreen(@Nullable Intent initialIntent)
  {
    SplashActivity.start(this, getClass(), initialIntent);
    finish();
  }
}
