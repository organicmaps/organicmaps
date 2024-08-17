package app.organicmaps.settings;

import android.content.Context;
import android.util.AttributeSet;

import androidx.annotation.NonNull;
import androidx.preference.ListPreference;
import androidx.preference.PreferenceViewHolder;

public class CustomListPreference extends ListPreference
{
  private CustomPreference.TextViews mTextViews = new CustomPreference.TextViews();

  public CustomListPreference(Context ctx, AttributeSet attrs, int defStyle)
  {
    super(ctx, attrs, defStyle);
  }

  public CustomListPreference(Context ctx, AttributeSet attrs)
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
