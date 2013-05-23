
package org.holoeverywhere.app;

import org.holoeverywhere.R;
import org.holoeverywhere.internal.AlertController;
import org.holoeverywhere.internal.AlertController.AlertDecorViewInstaller;
import org.holoeverywhere.internal.AlertController.AlertParams;
import org.holoeverywhere.internal.AlertController.AlertParams.OnPrepareListViewListener;
import org.holoeverywhere.widget.Button;
import org.holoeverywhere.widget.ListView;

import android.content.Context;
import android.content.DialogInterface;
import android.database.Cursor;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.os.Message;
import android.util.TypedValue;
import android.view.KeyEvent;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ListAdapter;

public class AlertDialog extends Dialog implements DialogInterface,
        AlertDecorViewInstaller {
    public static class Builder {
        private static final Class<?>[] CONSTRUCTOR_SIGNATURE = {
                Context.class, int.class
        };
        private Class<? extends AlertDialog> mDialogClass;
        private final AlertController.AlertParams mParams;

        public Builder(Context context) {
            this(context, 0);
        }

        public Builder(Context context, int theme) {
            mParams = new AlertParams(context, theme);
        }

        public Builder addButtonBehavior(int buttonBehavior) {
            mParams.mButtonBehavior |= buttonBehavior;
            return this;
        }

        public AlertDialog create() {
            AlertDialog dialog = null;
            if (mDialogClass != null) {
                try {
                    dialog = mDialogClass.getConstructor(CONSTRUCTOR_SIGNATURE).newInstance(
                            mParams.mContext, mParams.mTheme);
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
            if (dialog == null) {
                dialog = new AlertDialog(mParams.mContext, mParams.mTheme);
            }
            mParams.apply(dialog.mAlert);
            dialog.setCancelable(mParams.mCancelable);
            if (mParams.mCancelable) {
                dialog.setCanceledOnTouchOutside(true);
            }
            if (mParams.mOnCancelListener != null) {
                dialog.setOnCancelListener(mParams.mOnCancelListener);
            }
            if (mParams.mOnKeyListener != null) {
                dialog.setOnKeyListener(mParams.mOnKeyListener);
            }
            if (mParams.mOnDismissListener != null) {
                dialog.setOnDismissListener(mParams.mOnDismissListener);
            }
            return dialog;
        }

        public Context getContext() {
            return mParams.mContext;
        }

        public Builder removeButtonBehavior(int buttonBehavior) {
            mParams.mButtonBehavior |= buttonBehavior;
            mParams.mButtonBehavior ^= buttonBehavior;
            return this;
        }

        public Builder setAdapter(final ListAdapter adapter,
                final OnClickListener listener) {
            mParams.mAdapter = adapter;
            mParams.mOnClickListener = listener;
            return this;
        }

        public Builder setAlertDialogClass(Class<? extends AlertDialog> clazz) {
            mDialogClass = clazz;
            return this;
        }

        public Builder setBlockDismiss(boolean blockDismiss) {
            return setButtonBehavior(blockDismiss ? 0 : DISMISS_ON_ALL);
        }

        public Builder setButtonBehavior(int buttonBehavior) {
            mParams.mButtonBehavior = buttonBehavior;
            return this;
        }

        public Builder setCancelable(boolean cancelable) {
            mParams.mCancelable = cancelable;
            return this;
        }

        public Builder setCheckedItem(int checkedItem) {
            mParams.mCheckedItem = checkedItem;
            return this;
        }

        public Builder setCursor(final Cursor cursor,
                final OnClickListener listener, String labelColumn) {
            mParams.mCursor = cursor;
            mParams.mLabelColumn = labelColumn;
            mParams.mOnClickListener = listener;
            return this;
        }

        public Builder setCustomTitle(View customTitleView) {
            mParams.mCustomTitleView = customTitleView;
            return this;
        }

        public Builder setIcon(Drawable icon) {
            mParams.mIcon = icon;
            return this;
        }

        public Builder setIcon(int iconId) {
            mParams.mIconId = iconId;
            return this;
        }

        public Builder setIconAttribute(int attrId) {
            TypedValue out = new TypedValue();
            mParams.mContext.getTheme().resolveAttribute(attrId, out, true);
            mParams.mIconId = out.resourceId;
            return this;
        }

        public Builder setInverseBackgroundForced(boolean useInverseBackground) {
            mParams.mForceInverseBackground = useInverseBackground;
            return this;
        }

        public Builder setItems(CharSequence[] items,
                final OnClickListener listener) {
            mParams.mItems = items;
            mParams.mOnClickListener = listener;
            return this;
        }

        public Builder setItems(int itemsId, final OnClickListener listener) {
            mParams.mItems = mParams.mContext.getResources().getTextArray(itemsId);
            mParams.mOnClickListener = listener;
            return this;
        }

        public Builder setMessage(CharSequence message) {
            mParams.mMessage = message;
            return this;
        }

        public Builder setMessage(int messageId) {
            mParams.mMessage = mParams.mContext.getText(messageId);
            return this;
        }

        public Builder setMultiChoiceItems(CharSequence[] items,
                boolean[] checkedItems,
                final OnMultiChoiceClickListener listener) {
            mParams.mItems = items;
            mParams.mOnCheckboxClickListener = listener;
            mParams.mCheckedItems = checkedItems;
            mParams.mIsMultiChoice = true;
            return this;
        }

        public Builder setMultiChoiceItems(Cursor cursor,
                String isCheckedColumn, String labelColumn,
                final OnMultiChoiceClickListener listener) {
            mParams.mCursor = cursor;
            mParams.mOnCheckboxClickListener = listener;
            mParams.mIsCheckedColumn = isCheckedColumn;
            mParams.mLabelColumn = labelColumn;
            mParams.mIsMultiChoice = true;
            return this;
        }

        public Builder setMultiChoiceItems(int itemsId, boolean[] checkedItems,
                final OnMultiChoiceClickListener listener) {
            mParams.mItems = mParams.mContext.getResources().getTextArray(itemsId);
            mParams.mOnCheckboxClickListener = listener;
            mParams.mCheckedItems = checkedItems;
            mParams.mIsMultiChoice = true;
            return this;
        }

        public Builder setNegativeButton(CharSequence text,
                final OnClickListener listener) {
            mParams.mNegativeButtonText = text;
            mParams.mNegativeButtonListener = listener;
            return this;
        }

        public Builder setNegativeButton(int textId,
                final OnClickListener listener) {
            mParams.mNegativeButtonText = mParams.mContext.getText(textId);
            mParams.mNegativeButtonListener = listener;
            return this;
        }

        public Builder setNeutralButton(CharSequence text,
                final OnClickListener listener) {
            mParams.mNeutralButtonText = text;
            mParams.mNeutralButtonListener = listener;
            return this;
        }

        public Builder setNeutralButton(int textId,
                final OnClickListener listener) {
            mParams.mNeutralButtonText = mParams.mContext.getText(textId);
            mParams.mNeutralButtonListener = listener;
            return this;
        }

        public Builder setOnCancelListener(OnCancelListener onCancelListener) {
            mParams.mOnCancelListener = onCancelListener;
            return this;
        }

        public Builder setOnDismissListener(OnDismissListener onDismissListener) {
            mParams.mOnDismissListener = onDismissListener;
            return this;
        }

        public Builder setOnItemSelectedListener(
                final AdapterView.OnItemSelectedListener listener) {
            mParams.mOnItemSelectedListener = listener;
            return this;
        }

        public Builder setOnKeyListener(OnKeyListener onKeyListener) {
            mParams.mOnKeyListener = onKeyListener;
            return this;
        }

        public Builder setOnPrepareListViewListener(
                OnPrepareListViewListener listener) {
            mParams.mOnPrepareListViewListener = listener;
            return this;
        }

        public Builder setPositiveButton(CharSequence text,
                final OnClickListener listener) {
            mParams.mPositiveButtonText = text;
            mParams.mPositiveButtonListener = listener;
            return this;
        }

        public Builder setPositiveButton(int textId,
                final OnClickListener listener) {
            mParams.mPositiveButtonText = mParams.mContext.getText(textId);
            mParams.mPositiveButtonListener = listener;
            return this;
        }

        public Builder setSingleChoiceItems(CharSequence[] items,
                int checkedItem, final OnClickListener listener) {
            mParams.mItems = items;
            mParams.mOnClickListener = listener;
            mParams.mCheckedItem = checkedItem;
            mParams.mIsSingleChoice = true;
            return this;
        }

        public Builder setSingleChoiceItems(Cursor cursor, int checkedItem,
                String labelColumn, final OnClickListener listener) {
            mParams.mCursor = cursor;
            mParams.mOnClickListener = listener;
            mParams.mCheckedItem = checkedItem;
            mParams.mLabelColumn = labelColumn;
            mParams.mIsSingleChoice = true;
            return this;
        }

        public Builder setSingleChoiceItems(int itemsId, int checkedItem,
                final OnClickListener listener) {
            mParams.mItems = mParams.mContext.getResources().getTextArray(itemsId);
            mParams.mOnClickListener = listener;
            mParams.mCheckedItem = checkedItem;
            mParams.mIsSingleChoice = true;
            return this;
        }

        public Builder setSingleChoiceItems(ListAdapter adapter,
                int checkedItem, final OnClickListener listener) {
            mParams.mAdapter = adapter;
            mParams.mOnClickListener = listener;
            mParams.mCheckedItem = checkedItem;
            mParams.mIsSingleChoice = true;
            return this;
        }

        public Builder setTheme(int theme) {
            mParams.mTheme = theme;
            return this;
        }

        public Builder setTitle(CharSequence title) {
            mParams.mTitle = title;
            return this;
        }

        public Builder setTitle(int titleId) {
            mParams.mTitle = mParams.mContext.getText(titleId);
            return this;
        }

        public Builder setView(View view) {
            mParams.mView = view;
            mParams.mViewSpacingSpecified = false;
            return this;
        }

        public Builder setView(View view, int viewSpacingLeft,
                int viewSpacingTop, int viewSpacingRight, int viewSpacingBottom) {
            mParams.mView = view;
            mParams.mViewSpacingSpecified = true;
            mParams.mViewSpacingLeft = viewSpacingLeft;
            mParams.mViewSpacingTop = viewSpacingTop;
            mParams.mViewSpacingRight = viewSpacingRight;
            mParams.mViewSpacingBottom = viewSpacingBottom;
            return this;
        }

        public AlertDialog show() {
            AlertDialog dialog = create();
            dialog.show();
            return dialog;
        }
    }

    public static final int DISMISS_ON_ALL = 7; // DO_NEG | DO_NEU | DO_POS
    public static final int DISMISS_ON_NEGATIVE = 1 << 0; // -BUTTON_NEGATIVE;
    public static final int DISMISS_ON_NEUTRAL = 1 << 1; // -BUTTON_NEUTRAL;
    public static final int DISMISS_ON_POSITIVE = 1 << 2; // -BUTTON_POSITIVE;

    public static final int THEME_HOLO_DARK = 1;
    public static final int THEME_HOLO_LIGHT = 2;

    static int resolveDialogTheme(Context context, int resid) {
        if (resid == AlertDialog.THEME_HOLO_DARK) {
            return R.style.Holo_Theme_Dialog_Alert;
        } else if (resid == AlertDialog.THEME_HOLO_LIGHT) {
            return R.style.Holo_Theme_Dialog_Alert_Light;
        } else if (resid >= 0x01000000) {
            return resid;
        } else {
            TypedValue outValue = new TypedValue();
            context.getTheme().resolveAttribute(R.attr.alertDialogTheme, outValue, true);
            return outValue.resourceId;
        }
    }

    private final AlertController mAlert;

    protected AlertDialog(Context context) {
        this(context, true, null, 0);
    }

    protected AlertDialog(Context context, boolean cancelable,
            OnCancelListener cancelListener) {
        this(context, cancelable, cancelListener, 0);
    }

    protected AlertDialog(Context context, boolean cancelable,
            OnCancelListener cancelListener, int theme) {
        super(context, AlertDialog.resolveDialogTheme(context, theme));
        setCancelable(cancelable);
        setOnCancelListener(cancelListener);
        mAlert = new AlertController(getContext(), this, getWindow(), this);
    }

    protected AlertDialog(Context context, int theme) {
        this(context, true, null, theme);
    }

    public Button getButton(int whichButton) {
        return mAlert.getButton(whichButton);
    }

    public ListView getListView() {
        return mAlert.getListView();
    }

    @Override
    public void installDecorView(Context context, int layout) {
        setContentView(layout);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mAlert.installContent();
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (mAlert.onKeyDown(keyCode, event)) {
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        if (mAlert.onKeyUp(keyCode, event)) {
            return true;
        }
        return super.onKeyUp(keyCode, event);
    }

    @Deprecated
    public void setButton(CharSequence text, Message msg) {
        setButton(DialogInterface.BUTTON_POSITIVE, text, msg);
    }

    @Deprecated
    public void setButton(CharSequence text, final OnClickListener listener) {
        setButton(DialogInterface.BUTTON_POSITIVE, text, listener);
    }

    public void setButton(int whichButton, CharSequence text, Message msg) {
        mAlert.setButton(whichButton, text, null, msg);
    }

    public void setButton(int whichButton, CharSequence text,
            OnClickListener listener) {
        mAlert.setButton(whichButton, text, listener, null);
    }

    @Deprecated
    public void setButton2(CharSequence text, Message msg) {
        setButton(DialogInterface.BUTTON_NEGATIVE, text, msg);
    }

    @Deprecated
    public void setButton2(CharSequence text, final OnClickListener listener) {
        setButton(DialogInterface.BUTTON_NEGATIVE, text, listener);
    }

    @Deprecated
    public void setButton3(CharSequence text, Message msg) {
        setButton(DialogInterface.BUTTON_NEUTRAL, text, msg);
    }

    @Deprecated
    public void setButton3(CharSequence text, final OnClickListener listener) {
        setButton(DialogInterface.BUTTON_NEUTRAL, text, listener);
    }

    public void setButtonBehavior(int buttonBehavior) {
        mAlert.setButtonBehavior(buttonBehavior);
    }

    public void setCustomTitle(View customTitleView) {
        mAlert.setCustomTitle(customTitleView);
    }

    public void setIcon(Drawable icon) {
        mAlert.setIcon(icon);
    }

    public void setIcon(int resId) {
        mAlert.setIcon(resId);
    }

    public void setIconAttribute(int attrId) {
        TypedValue out = new TypedValue();
        getContext().getTheme().resolveAttribute(attrId, out, true);
        mAlert.setIcon(out.resourceId);
    }

    public void setInverseBackgroundForced(boolean forceInverseBackground) {
        mAlert.setInverseBackgroundForced(forceInverseBackground);
    }

    public void setMessage(CharSequence message) {
        mAlert.setMessage(message);
    }

    @Override
    public void setTitle(CharSequence title) {
        super.setTitle(title);
        mAlert.setTitle(title);
    }

    public void setView(View view) {
        mAlert.setView(view);
    }

    public void setView(View view, int viewSpacingLeft, int viewSpacingTop,
            int viewSpacingRight, int viewSpacingBottom) {
        mAlert.setView(view, viewSpacingLeft, viewSpacingTop, viewSpacingRight,
                viewSpacingBottom);
    }
}
