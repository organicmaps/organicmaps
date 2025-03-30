package app.organicmaps.sync.nc;

import android.content.Context;
import android.util.AttributeSet;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.preference.SwitchPreference;

public class SyncEnabledSwitchPreference extends SwitchPreference
{
  private NextcloudPreferences ncprefs;

  public SyncEnabledSwitchPreference(@NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr, int defStyleRes)
  {
    super(context, attrs, defStyleAttr, defStyleRes);
  }

  public SyncEnabledSwitchPreference(@NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr)
  {
    super(context, attrs, defStyleAttr);
  }

  public SyncEnabledSwitchPreference(@NonNull Context context, @Nullable AttributeSet attrs)
  {
    super(context, attrs);
  }

  public SyncEnabledSwitchPreference(@NonNull Context context)
  {
    super(context);
  }

  @Override
  protected void onSetInitialValue(Object defaultValue)
  {
    ncprefs = new NextcloudPreferences(getContext());
    setChecked(ncprefs.getSyncEnabled());
  }

  @Override
  protected boolean persistBoolean(boolean value)
  {
    ncprefs.setSyncEnabled(value);
    return true;
  }
}
