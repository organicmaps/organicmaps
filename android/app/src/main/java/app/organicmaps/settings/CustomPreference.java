package app.organicmaps.settings;

import android.content.Context;
import android.graphics.Color;
import android.util.AttributeSet;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.preference.Preference;
import androidx.preference.PreferenceViewHolder;

public class CustomPreference extends Preference
{
  public static class TextViews
  {
    TextView mTitleTextView;
    int mDefaultTitleTextColor;

    TextView mSummaryTextView;
    int mDefaultSummaryTextColor;

    public void init(PreferenceViewHolder holder)
    {
      mTitleTextView = (TextView) holder.findViewById(android.R.id.title);
      if (mTitleTextView != null)
        mDefaultTitleTextColor = mTitleTextView.getTextColors().getDefaultColor();

      mSummaryTextView = (TextView) holder.findViewById(android.R.id.summary);
      if (mSummaryTextView != null)
        mDefaultSummaryTextColor = mSummaryTextView.getTextColors().getDefaultColor();
    }

    public void updateTextViewColors(boolean enabled)
    {
      if (mTitleTextView != null)
        mTitleTextView.setTextColor(enabled? mDefaultTitleTextColor : Color.GRAY);

      if (mSummaryTextView != null)
        mSummaryTextView.setTextColor(enabled? mDefaultSummaryTextColor : Color.GRAY);
    }
  }

  TextViews mTextViews = new TextViews();

  public CustomPreference(Context ctx, AttributeSet attrs, int defStyle)
  {
    super(ctx, attrs, defStyle);
  }

  public CustomPreference(Context ctx, AttributeSet attrs)
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
