
package org.holoeverywhere.app;

import java.text.NumberFormat;

import org.holoeverywhere.R;
import org.holoeverywhere.widget.ProgressBar;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.style.StyleSpan;
import android.view.View;
import android.widget.TextView;

public class ProgressDialog extends AlertDialog {
    public static final int STYLE_HORIZONTAL = 1;
    public static final int STYLE_SPINNER = 0;

    public static ProgressDialog show(Context context, CharSequence title,
            CharSequence message) {
        return ProgressDialog.show(context, title, message, false);
    }

    public static ProgressDialog show(Context context, CharSequence title,
            CharSequence message, boolean indeterminate) {
        return ProgressDialog.show(context, title, message, indeterminate,
                false, null);
    }

    public static ProgressDialog show(Context context, CharSequence title,
            CharSequence message, boolean indeterminate, boolean cancelable) {
        return ProgressDialog.show(context, title, message, indeterminate,
                cancelable, null);
    }

    public static ProgressDialog show(Context context, CharSequence title,
            CharSequence message, boolean indeterminate, boolean cancelable,
            OnCancelListener cancelListener) {
        ProgressDialog dialog = new ProgressDialog(context);
        dialog.setTitle(title);
        dialog.setMessage(message);
        dialog.setIndeterminate(indeterminate);
        dialog.setCancelable(cancelable);
        dialog.setOnCancelListener(cancelListener);
        dialog.show();
        return dialog;
    }

    private boolean mHasStarted;
    private int mIncrementBy;
    private int mIncrementSecondaryBy;
    private boolean mIndeterminate;
    private Drawable mIndeterminateDrawable;
    private int mMax;
    private CharSequence mMessage;
    private TextView mMessageView;
    private ProgressBar mProgress;
    private Drawable mProgressDrawable;
    private TextView mProgressNumber;
    private String mProgressNumberFormat;
    private TextView mProgressPercent;
    private NumberFormat mProgressPercentFormat;
    private int mProgressStyle = ProgressDialog.STYLE_SPINNER;
    private int mProgressVal;
    private int mSecondaryProgressVal;
    private Handler mViewUpdateHandler;

    public ProgressDialog(Context context) {
        super(context);
        initFormats();
    }

    public ProgressDialog(Context context, int theme) {
        super(context, theme);
        initFormats();
    }

    public int getMax() {
        if (mProgress != null) {
            return mProgress.getMax();
        }
        return mMax;
    }

    public int getProgress() {
        if (mProgress != null) {
            return mProgress.getProgress();
        }
        return mProgressVal;
    }

    public int getSecondaryProgress() {
        if (mProgress != null) {
            return mProgress.getSecondaryProgress();
        }
        return mSecondaryProgressVal;
    }

    public void incrementProgressBy(int diff) {
        if (mProgress != null) {
            mProgress.incrementProgress(diff);
            onProgressChanged();
        } else {
            mIncrementBy += diff;
        }
    }

    public void incrementSecondaryProgressBy(int diff) {
        if (mProgress != null) {
            mProgress.incrementSecondaryProgress(diff);
            onProgressChanged();
        } else {
            mIncrementSecondaryBy += diff;
        }
    }

    private void initFormats() {
        mProgressNumberFormat = "%1d/%2d";
        mProgressPercentFormat = NumberFormat.getPercentInstance();
        mProgressPercentFormat.setMaximumFractionDigits(0);
    }

    public boolean isIndeterminate() {
        if (mProgress != null) {
            return mProgress.isIndeterminate();
        }
        return mIndeterminate;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        TypedArray a = getContext().obtainStyledAttributes(null,
                R.styleable.AlertDialog, R.attr.alertDialogStyle,
                R.style.Holo_AlertDialog);
        View view;
        if (mProgressStyle == ProgressDialog.STYLE_HORIZONTAL) {
            mViewUpdateHandler = new Handler() {
                @Override
                public void handleMessage(Message msg) {
                    super.handleMessage(msg);
                    int progress = mProgress.getProgress();
                    int max = mProgress.getMax();
                    if (mProgressNumberFormat != null) {
                        String format = mProgressNumberFormat;
                        mProgressNumber.setText(String.format(format, progress,
                                max));
                    } else {
                        mProgressNumber.setText("");
                    }
                    if (mProgressPercentFormat != null) {
                        double percent = (double) progress / (double) max;
                        SpannableString tmp = new SpannableString(
                                mProgressPercentFormat.format(percent));
                        tmp.setSpan(new StyleSpan(
                                android.graphics.Typeface.BOLD), 0, tmp
                                .length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                        mProgressPercent.setText(tmp);
                    } else {
                        mProgressPercent.setText("");
                    }
                }
            };
            view = getLayoutInflater().inflate(
                    a.getResourceId(
                            R.styleable.AlertDialog_horizontalProgressLayout,
                            R.layout.alert_dialog_progress_holo));
            mProgress = (ProgressBar) view.findViewById(R.id.progress);
            mProgressNumber = (TextView) view
                    .findViewById(R.id.progress_number);
            mProgressPercent = (TextView) view
                    .findViewById(R.id.progress_percent);
        } else {
            view = getLayoutInflater().inflate(
                    a.getResourceId(R.styleable.AlertDialog_progressLayout,
                            R.layout.progress_dialog_holo));
            mProgress = (ProgressBar) view.findViewById(R.id.progress);
            mMessageView = (TextView) view.findViewById(R.id.message);
        }
        setView(view);
        a.recycle();
        if (mMax > 0) {
            setMax(mMax);
        }
        if (mProgressVal > 0) {
            setProgress(mProgressVal);
        }
        if (mSecondaryProgressVal > 0) {
            setSecondaryProgress(mSecondaryProgressVal);
        }
        if (mIncrementBy > 0) {
            incrementProgressBy(mIncrementBy);
        }
        if (mIncrementSecondaryBy > 0) {
            incrementSecondaryProgressBy(mIncrementSecondaryBy);
        }
        if (mProgressDrawable != null) {
            setProgressDrawable(mProgressDrawable);
        }
        if (mIndeterminateDrawable != null) {
            setIndeterminateDrawable(mIndeterminateDrawable);
        }
        if (mMessage != null) {
            setMessage(mMessage);
        }
        setIndeterminate(mIndeterminate);
        onProgressChanged();
        super.onCreate(savedInstanceState);
    }

    private void onProgressChanged() {
        if (mProgressStyle == ProgressDialog.STYLE_HORIZONTAL) {
            if (mViewUpdateHandler != null
                    && !mViewUpdateHandler.hasMessages(0)) {
                mViewUpdateHandler.sendEmptyMessage(0);
            }
        }
    }

    @Override
    public void onStart() {
        super.onStart();
        mHasStarted = true;
    }

    @Override
    protected void onStop() {
        super.onStop();
        mHasStarted = false;
    }

    public void setIndeterminate(boolean indeterminate) {
        if (mProgress != null) {
            mProgress.setIndeterminate(indeterminate);
        } else {
            mIndeterminate = indeterminate;
        }
    }

    public void setIndeterminateDrawable(Drawable d) {
        if (mProgress != null) {
            mProgress.setIndeterminateDrawable(d);
        } else {
            mIndeterminateDrawable = d;
        }
    }

    public void setMax(int max) {
        if (mProgress != null) {
            mProgress.setMax(max);
            onProgressChanged();
        } else {
            mMax = max;
        }
    }

    @Override
    public void setMessage(CharSequence message) {
        if (mProgress != null) {
            if (mProgressStyle == ProgressDialog.STYLE_HORIZONTAL) {
                super.setMessage(message);
            } else {
                mMessageView.setText(message);
            }
        } else {
            mMessage = message;
        }
    }

    public void setProgress(int value) {
        if (mHasStarted) {
            mProgress.setProgress(value);
            onProgressChanged();
        } else {
            mProgressVal = value;
        }
    }

    public void setProgressDrawable(Drawable d) {
        if (mProgress != null) {
            mProgress.setProgressDrawable(d);
        } else {
            mProgressDrawable = d;
        }
    }

    public void setProgressNumberFormat(String format) {
        mProgressNumberFormat = format;
        onProgressChanged();
    }

    public void setProgressPercentFormat(NumberFormat format) {
        mProgressPercentFormat = format;
        onProgressChanged();
    }

    public void setProgressStyle(int style) {
        mProgressStyle = style;
    }

    public void setSecondaryProgress(int secondaryProgress) {
        if (mProgress != null) {
            mProgress.setSecondaryProgress(secondaryProgress);
            onProgressChanged();
        } else {
            mSecondaryProgressVal = secondaryProgress;
        }
    }
}
