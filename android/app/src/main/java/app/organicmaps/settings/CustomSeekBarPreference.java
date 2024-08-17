package app.organicmaps.settings;

import android.content.Context;
import android.graphics.Color;
import android.util.AttributeSet;

import androidx.annotation.NonNull;
import androidx.preference.PreferenceViewHolder;
import androidx.preference.SeekBarPreference;

public class CustomSeekBarPreference extends SeekBarPreference
{
  private CustomPreference.TextViews mTextViews = new CustomPreference.TextViews();

  public CustomSeekBarPreference(Context ctx, AttributeSet attrs, int defStyle)
  {
    super(ctx, attrs, defStyle);
  }

  public CustomSeekBarPreference(Context ctx, AttributeSet attrs)
  {
    super(ctx, attrs);
  }

  @Override
  public void onBindViewHolder(@NonNull PreferenceViewHolder holder)
  {
    super.onBindViewHolder(holder);

    mTextViews.init(holder);

    mTextViews.updateTextViewColors(isEnabled());
  }

  @Override
  public void setEnabled(boolean enabled)
  {
    super.setEnabled(enabled);

    mTextViews.updateTextViewColors(enabled);
  }
}
