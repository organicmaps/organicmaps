package com.mapswithme.maps.base;

import android.app.Activity;
import android.media.AudioManager;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.MenuItem;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.Utils;

public class BaseMwmFragmentActivity extends AppCompatActivity
                                  implements BaseActivity
{
  private final BaseActivityDelegate mBaseDelegate = new BaseActivityDelegate(this);

  @Override
  public Activity get()
  {
    return this;
  }

  @Override
  public int getThemeResourceId(String theme)
  {
    if (ThemeUtils.isDefaultTheme(theme))
        return R.style.MwmTheme;

    if (ThemeUtils.isNightTheme(theme))
      return R.style.MwmTheme_Night;

    throw new IllegalArgumentException("Attempt to apply unsupported theme: " + theme);
  }

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    mBaseDelegate.onCreate();

    super.onCreate(savedInstanceState);
    setVolumeControlStream(AudioManager.STREAM_MUSIC);
    final int layoutId = getContentLayoutResId();
    if (layoutId != 0)
      setContentView(layoutId);

    // Use full-screen on Kindle Fire only
    if (Utils.isAmazonDevice())
    {
      getWindow().addFlags(android.view.WindowManager.LayoutParams.FLAG_FULLSCREEN);
      getWindow().clearFlags(android.view.WindowManager.LayoutParams.FLAG_FORCE_NOT_FULLSCREEN);
    }

    MwmApplication.get().initNativeCore();
    MwmApplication.get().initCounters();

    attachDefaultFragment();
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
  }

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
      onBackPressed();
      return true;
    }
    return super.onOptionsItemSelected(item);
  }

  @Override
  protected void onResume()
  {
    super.onResume();
    mBaseDelegate.onResume();
  }

  @Override
  protected void onPostResume() {
    super.onPostResume();
    mBaseDelegate.onPostResume();
  }

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
  public void replaceFragment(Class<? extends Fragment> fragmentClass, Bundle args, @Nullable Runnable completionListener)
  {
    final int resId = getFragmentContentResId();
    if (resId <= 0 || findViewById(resId) == null)
      throw new IllegalStateException("Fragment can't be added, since getFragmentContentResId() isn't implemented or returns wrong resourceId.");

    String name = fragmentClass.getName();
    final Fragment fragment = Fragment.instantiate(this, name, args);
    getSupportFragmentManager().beginTransaction()
                               .replace(resId, fragment, name)
                               .commit();
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
