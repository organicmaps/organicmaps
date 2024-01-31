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
  private int mTextViewId;
  private OnBindTextViewListener mOnBindTextViewListener = null;

  public CustomPreference(Context ctx, AttributeSet attrs, int defStyle)
  {
    super(ctx, attrs, defStyle);
  }

  public CustomPreference(Context ctx, AttributeSet attrs)
  {
    super(ctx, attrs);
  }

  public interface OnBindTextViewListener
  {
    void onBindTextView(TextView textView);
  }

  public void setOnBindTextViewListener(int textViewId, OnBindTextViewListener listener)
  {
    mTextViewId = textViewId;
    mOnBindTextViewListener = listener;
  }

  @Override
  public void onBindViewHolder(@NonNull PreferenceViewHolder holder)
  {
    super.onBindViewHolder(holder);

    if (mOnBindTextViewListener != null)
    {
      TextView textViewSummary = (TextView) holder.findViewById(mTextViewId);
      mOnBindTextViewListener.onBindTextView(textViewSummary);
    }
  }
}
