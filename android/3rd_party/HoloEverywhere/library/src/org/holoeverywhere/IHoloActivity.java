
package org.holoeverywhere;

import org.holoeverywhere.SystemServiceManager.SuperSystemService;
import org.holoeverywhere.ThemeManager.SuperStartActivity;
import org.holoeverywhere.addon.IAddonActivity;
import org.holoeverywhere.addon.IAddonAttacher;

import android.support.v4.app.FragmentManager;

import com.actionbarsherlock.ActionBarSherlock.OnActionModeFinishedListener;
import com.actionbarsherlock.ActionBarSherlock.OnActionModeStartedListener;
import com.actionbarsherlock.ActionBarSherlock.OnCreatePanelMenuListener;
import com.actionbarsherlock.ActionBarSherlock.OnMenuItemSelectedListener;
import com.actionbarsherlock.ActionBarSherlock.OnPreparePanelListener;
import com.actionbarsherlock.app.ActionBar;
import com.actionbarsherlock.internal.view.menu.ContextMenuDecorView.ContextMenuListenersProvider;
import com.actionbarsherlock.internal.view.menu.ContextMenuListener;
import com.actionbarsherlock.view.ActionMode;
import com.actionbarsherlock.view.Menu;
import com.actionbarsherlock.view.MenuInflater;
import com.actionbarsherlock.view.MenuItem;

public interface IHoloActivity extends IHolo, SuperStartActivity,
        OnCreatePanelMenuListener, OnPreparePanelListener,
        OnMenuItemSelectedListener, OnActionModeStartedListener,
        OnActionModeFinishedListener, SuperSystemService, ContextMenuListener,
        ContextMenuListenersProvider, IAddonAttacher<IAddonActivity> {
    public static interface OnWindowFocusChangeListener {
        public void onWindowFocusChanged(boolean hasFocus);
    }

    public void addOnWindowFocusChangeListener(OnWindowFocusChangeListener listener);

    public ActionBar getSupportActionBar();

    public FragmentManager getSupportFragmentManager();

    public MenuInflater getSupportMenuInflater();

    public boolean isForceThemeApply();

    public boolean onCreateOptionsMenu(Menu menu);

    public boolean onOptionsItemSelected(MenuItem item);

    public boolean onPrepareOptionsMenu(Menu menu);

    public void requestWindowFeature(long featureId);

    public void setSupportProgress(int progress);

    public void setSupportProgressBarIndeterminate(boolean indeterminate);

    public void setSupportProgressBarIndeterminateVisibility(boolean visible);

    public void setSupportProgressBarVisibility(boolean visible);

    public void setSupportSecondaryProgress(int secondaryProgress);

    public ActionMode startActionMode(ActionMode.Callback callback);

    public android.content.SharedPreferences superGetSharedPreferences(
            String name, int mode);

    public void supportInvalidateOptionsMenu();
}
