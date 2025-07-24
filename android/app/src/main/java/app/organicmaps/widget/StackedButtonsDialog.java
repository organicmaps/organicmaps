package app.organicmaps.widget;

import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.view.View;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.appcompat.app.AppCompatDialog;
import app.organicmaps.R;
import app.organicmaps.util.UiUtils;

public class StackedButtonsDialog extends AppCompatDialog implements View.OnClickListener
{
  @Nullable
  private final String mTitle;
  @Nullable
  private final String mMessage;
  @Nullable
  private final String mPositive;
  @Nullable
  private final DialogInterface.OnClickListener mPositiveListener;
  @Nullable
  private final String mNeutral;
  @Nullable
  private final DialogInterface.OnClickListener mNeutralListener;
  @Nullable
  private final String mNegative;
  @Nullable
  private final DialogInterface.OnClickListener mNegativeListener;
  private final boolean mCancelable;
  @Nullable
  private final OnCancelListener mCancelListener;

  private StackedButtonsDialog(Context context, @Nullable String title, @Nullable String message,
                               @Nullable String positive, @Nullable OnClickListener positiveListener,
                               @Nullable String neutral, @Nullable OnClickListener neutralListener,
                               @Nullable String negative, @Nullable OnClickListener negativeListener,
                               boolean cancelable, @Nullable OnCancelListener cancelListener)
  {
    super(context);
    mTitle = title;
    mMessage = message;
    mPositive = positive;
    mPositiveListener = positiveListener;
    mNeutral = neutral;
    mNeutralListener = neutralListener;
    mNegative = negative;
    mNegativeListener = negativeListener;
    mCancelable = cancelable;
    mCancelListener = cancelListener;
  }

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    setCancelable(mCancelable);
    setOnCancelListener(mCancelListener);
    setContentView(R.layout.dialog_stacked_buttons);

    TextView title = findViewById(R.id.tv__title);
    UiUtils.setTextAndHideIfEmpty(title, mTitle);
    TextView message = findViewById(R.id.tv__message);
    UiUtils.setTextAndHideIfEmpty(message, mMessage);
    TextView positive = findViewById(R.id.btn__positive);
    positive.setOnClickListener(this);
    UiUtils.setTextAndHideIfEmpty(positive, mPositive);
    TextView neutral = findViewById(R.id.btn__neutral);
    neutral.setOnClickListener(this);
    UiUtils.setTextAndHideIfEmpty(neutral, mNeutral);
    TextView negative = findViewById(R.id.btn__negative);
    negative.setOnClickListener(this);
    UiUtils.setTextAndHideIfEmpty(negative, mNegative);
  }

  @Override
  public void onClick(View v)
  {
    final int id = v.getId();
    if (id == R.id.btn__positive)
    {
      if (mPositiveListener != null)
        mPositiveListener.onClick(this, DialogInterface.BUTTON_POSITIVE);
      dismiss();
    }
    else if (id == R.id.btn__neutral)
    {
      if (mNeutralListener != null)
        mNeutralListener.onClick(this, DialogInterface.BUTTON_NEUTRAL);
      dismiss();
    }
    else if (id == R.id.btn__negative)
    {
      if (mNegativeListener != null)
        mNegativeListener.onClick(this, DialogInterface.BUTTON_NEGATIVE);
      dismiss();
    }
  }

  public static final class Builder
  {
    @NonNull
    private final Context mContext;

    @Nullable
    private String mTitle;
    @Nullable
    private String mMessage;
    @Nullable
    private String mPositive;
    @Nullable
    private DialogInterface.OnClickListener mPositiveListener;
    @Nullable
    private String mNeutral;
    @Nullable
    private DialogInterface.OnClickListener mNeutralListener;
    @Nullable
    private String mNegative;
    @Nullable
    private DialogInterface.OnClickListener mNegativeListener;
    private boolean mCancelable = true;
    @Nullable
    private DialogInterface.OnCancelListener mCancelListener;

    public Builder(@NonNull Context context)
    {
      mContext = context;
      mTitle = mContext.getString(android.R.string.dialog_alert_title);
      mPositive = mContext.getString(R.string.ok);
      mNegative = mContext.getString(R.string.cancel);
    }

    @NonNull
    public Builder setTitle(@StringRes int titleId)
    {
      mTitle = mContext.getString(titleId);
      return this;
    }

    @NonNull
    public Builder setMessage(@StringRes int messageId)
    {
      mMessage = mContext.getString(messageId);
      return this;
    }

    @NonNull
    public Builder setPositiveButton(@StringRes int resId, @Nullable DialogInterface.OnClickListener listener)
    {
      mPositive = mContext.getString(resId);
      mPositiveListener = listener;
      return this;
    }

    @NonNull
    public Builder setNeutralButton(@StringRes int resId, @Nullable DialogInterface.OnClickListener listener)
    {
      mNeutral = mContext.getString(resId);
      mNeutralListener = listener;
      return this;
    }

    @NonNull
    public Builder setNegativeButton(@StringRes int resId, @Nullable DialogInterface.OnClickListener listener)
    {
      mNegative = mContext.getString(resId);
      mNegativeListener = listener;
      return this;
    }

    @NonNull
    public Builder setCancelable(boolean cancelable)
    {
      mCancelable = cancelable;
      return this;
    }

    @NonNull
    public StackedButtonsDialog build()
    {
      return new StackedButtonsDialog(mContext, mTitle, mMessage, mPositive, mPositiveListener, mNeutral,
                                      mNeutralListener, mNegative, mNegativeListener, mCancelable, mCancelListener);
    }
  }
}
