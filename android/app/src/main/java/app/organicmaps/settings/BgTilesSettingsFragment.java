package app.organicmaps.settings;

import android.os.Bundle;
import android.text.InputType;
import android.view.View;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.preference.EditTextPreference;
import androidx.preference.SeekBarPreference;
import androidx.preference.TwoStatePreference;
import app.organicmaps.R;
import app.organicmaps.sdk.Framework;

// Settings sub-screen for the custom raster background tiles (XYZ {z}/{x}/{y} source).
// Values are stored in the C++ core via Framework.native* and kept even while the layer is
// disabled; toggling "enabled" only switches the rendered mode.
public class BgTilesSettingsFragment extends BaseXmlSettingsFragment
{
  private static final int kMinCacheSizeMB = 1;
  private static final int kMaxCacheSizeMB = 1000;

  @NonNull
  @SuppressWarnings("NotNullFieldNotInitialized")
  private TwoStatePreference mEnabled;
  @NonNull
  @SuppressWarnings("NotNullFieldNotInitialized")
  private EditTextPreference mUrl;
  @NonNull
  @SuppressWarnings("NotNullFieldNotInitialized")
  private SeekBarPreference mCacheSize;

  @Override
  protected int getXmlResources()
  {
    return R.xml.prefs_bg_tiles;
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    mEnabled = getPreference(getString(R.string.pref_bg_tiles_enabled));
    mUrl = getPreference(getString(R.string.pref_bg_tiles_url));
    mCacheSize = getPreference(getString(R.string.pref_bg_tiles_size));

    mCacheSize.setMin(kMinCacheSizeMB);
    mCacheSize.setMax(kMaxCacheSizeMB);

    // Show a single-line URL field with the {z}/{x}/{y} example as the hint.
    mUrl.setOnBindEditTextListener(editText -> {
      editText.setHint(R.string.pref_bg_tiles_url_hint);
      // Plain text (not the URI variation): keep it a raw field so the "{z}/{x}/{y}" template
      // is easy to enter; disable suggestions/autocorrect.
      editText.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS);
      editText.setSingleLine(true);
      editText.setSelection(editText.getText().length());
    });

    // Load current values from the core.
    final boolean enabled = Framework.nativeIsBackgroundTilesEnabled();
    mEnabled.setChecked(enabled);
    mUrl.setText(Framework.nativeGetBackgroundTilesUrl());
    mCacheSize.setValue(Framework.nativeGetBackgroundTilesCacheSizeMB());
    updateFieldsEnabled(enabled);

    // Edits only update the in-memory preference values + field availability; everything is applied
    // together in onPause() when the user leaves this sub-screen.
    mEnabled.setOnPreferenceChangeListener((preference, newValue) -> {
      updateFieldsEnabled((Boolean) newValue);
      return true;
    });
  }

  @Override
  public void onPause()
  {
    Framework.nativeSetBackgroundTiles(mEnabled.isChecked(), currentUrl(), mCacheSize.getValue());
    super.onPause();
  }

  @NonNull
  private String currentUrl()
  {
    final String url = mUrl.getText();
    return url == null ? "" : url.trim();
  }

  // Grey out the URL / cache-size fields when the layer is disabled; their values are kept.
  private void updateFieldsEnabled(boolean enabled)
  {
    mUrl.setEnabled(enabled);
    mCacheSize.setEnabled(enabled);
  }
}
