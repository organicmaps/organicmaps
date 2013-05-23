
package org.holoeverywhere.internal;

import java.lang.ref.WeakReference;

import org.holoeverywhere.ArrayAdapter;
import org.holoeverywhere.LayoutInflater;
import org.holoeverywhere.R;
import org.holoeverywhere.app.AlertDialog;
import org.holoeverywhere.widget.Button;
import org.holoeverywhere.widget.CheckedTextView;
import org.holoeverywhere.widget.FrameLayout;
import org.holoeverywhere.widget.LinearLayout;
import org.holoeverywhere.widget.ListView;
import org.holoeverywhere.widget.TextView;

import android.content.Context;
import android.content.DialogInterface;
import android.content.res.TypedArray;
import android.database.Cursor;
import android.graphics.drawable.Drawable;
import android.os.Handler;
import android.os.Message;
import android.support.v4.widget.CursorAdapter;
import android.support.v4.widget.SimpleCursorAdapter;
import android.text.TextUtils;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.AbsListView;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.FrameLayout.LayoutParams;
import android.widget.ImageView;
import android.widget.ListAdapter;
import android.widget.ScrollView;

public class AlertController {
    public static interface AlertDecorViewInstaller {
        public void installDecorView(Context context, int layout);
    }

    public static class AlertParams {
        public interface OnPrepareListViewListener {
            void onPrepareListView(ListView listView);
        }

        public ListAdapter mAdapter;
        public int mButtonBehavior = AlertDialog.DISMISS_ON_ALL;
        public boolean mCancelable;
        public int mCheckedItem = -1;
        public boolean[] mCheckedItems;
        public final Context mContext;
        public Cursor mCursor;
        public View mCustomTitleView;
        public boolean mForceInverseBackground;
        public Drawable mIcon;
        public int mIconId = 0;
        public final LayoutInflater mInflater;
        public String mIsCheckedColumn;
        public boolean mIsMultiChoice;
        public boolean mIsSingleChoice;
        public CharSequence[] mItems;
        public String mLabelColumn;
        public CharSequence mMessage;
        public DialogInterface.OnClickListener mNegativeButtonListener;
        public CharSequence mNegativeButtonText;
        public DialogInterface.OnClickListener mNeutralButtonListener;
        public CharSequence mNeutralButtonText;
        public DialogInterface.OnCancelListener mOnCancelListener;
        public DialogInterface.OnMultiChoiceClickListener mOnCheckboxClickListener;
        public DialogInterface.OnClickListener mOnClickListener;
        public DialogInterface.OnDismissListener mOnDismissListener;
        public AdapterView.OnItemSelectedListener mOnItemSelectedListener;
        public DialogInterface.OnKeyListener mOnKeyListener;
        public OnPrepareListViewListener mOnPrepareListViewListener;
        public DialogInterface.OnClickListener mPositiveButtonListener;
        public CharSequence mPositiveButtonText;
        public int mTheme;
        public CharSequence mTitle;
        public View mView;
        public int mViewSpacingBottom;
        public int mViewSpacingLeft;
        public int mViewSpacingRight;
        public boolean mViewSpacingSpecified = false;
        public int mViewSpacingTop;

        public AlertParams(Context context) {
            this(context, 0);
        }

        public AlertParams(Context context, int theme) {
            mContext = context;
            mTheme = theme;
            mCancelable = true;
            mInflater = LayoutInflater.from(context);
        }

        public void apply(AlertController dialog) {
            if (mCustomTitleView != null) {
                dialog.setCustomTitle(mCustomTitleView);
            } else {
                if (mTitle != null) {
                    dialog.setTitle(mTitle);
                }
                if (mIcon != null) {
                    dialog.setIcon(mIcon);
                }
                if (mIconId >= 0) {
                    dialog.setIcon(mIconId);
                }
            }
            if (mMessage != null) {
                dialog.setMessage(mMessage);
            }
            if (mPositiveButtonText != null) {
                dialog.setButton(DialogInterface.BUTTON_POSITIVE,
                        mPositiveButtonText, mPositiveButtonListener, null);
            }
            if (mNegativeButtonText != null) {
                dialog.setButton(DialogInterface.BUTTON_NEGATIVE,
                        mNegativeButtonText, mNegativeButtonListener, null);
            }
            if (mNeutralButtonText != null) {
                dialog.setButton(DialogInterface.BUTTON_NEUTRAL,
                        mNeutralButtonText, mNeutralButtonListener, null);
            }
            dialog.setButtonBehavior(mButtonBehavior);
            if (mForceInverseBackground) {
                dialog.setInverseBackgroundForced(true);
            }
            if (mItems != null || mCursor != null || mAdapter != null) {
                createListView(dialog);
            }
            if (mView != null) {
                if (mViewSpacingSpecified) {
                    dialog.setView(mView, mViewSpacingLeft, mViewSpacingTop,
                            mViewSpacingRight, mViewSpacingBottom);
                } else {
                    dialog.setView(mView);
                }
            }
        }

        private void createListView(final AlertController dialog) {
            final ListView listView = (ListView) mInflater
                    .inflate(dialog.mListLayout, null);
            ListAdapter adapter;
            if (mIsMultiChoice) {
                if (mCursor == null) {
                    adapter = new ArrayAdapter<CharSequence>(mContext,
                            dialog.mMultiChoiceItemLayout, android.R.id.text1, mItems) {
                        @Override
                        public View getView(int position, View convertView,
                                ViewGroup parent) {
                            View view = super.getView(position, convertView,
                                    parent);
                            if (mCheckedItems != null) {
                                boolean isItemChecked = mCheckedItems[position];
                                if (isItemChecked) {
                                    listView.setItemChecked(position, true);
                                }
                            }
                            return view;
                        }
                    };
                } else {
                    adapter = new CursorAdapter(mContext, mCursor, false) {
                        private final int mIsCheckedIndex;
                        private final int mLabelIndex;

                        {
                            final Cursor cursor = getCursor();
                            mLabelIndex = cursor
                                    .getColumnIndexOrThrow(mLabelColumn);
                            mIsCheckedIndex = cursor
                                    .getColumnIndexOrThrow(mIsCheckedColumn);
                        }

                        @Override
                        public void bindView(View view, Context context,
                                Cursor cursor) {
                            CheckedTextView text = (CheckedTextView) view
                                    .findViewById(android.R.id.text1);
                            text.setText(cursor.getString(mLabelIndex));
                            listView.setItemChecked(cursor.getPosition(),
                                    cursor.getInt(mIsCheckedIndex) == 1);
                        }

                        @Override
                        public View newView(Context context, Cursor cursor,
                                ViewGroup parent) {
                            return mInflater.inflate(
                                    dialog.mMultiChoiceItemLayout, parent,
                                    false);
                        }

                    };
                }
            } else {
                int layout = mIsSingleChoice ? dialog.mSingleChoiceItemLayout
                        : dialog.mListItemLayout;
                if (mCursor == null) {
                    adapter = mAdapter != null ? mAdapter
                            : new ArrayAdapter<CharSequence>(mContext, layout,
                                    android.R.id.text1, mItems);
                } else {
                    adapter = new SimpleCursorAdapter(mContext, layout,
                            mCursor, new String[] {
                                    mLabelColumn
                            },
                            new int[] {
                                    android.R.id.text1
                            },
                            CursorAdapter.FLAG_REGISTER_CONTENT_OBSERVER);
                }
            }
            if (mOnPrepareListViewListener != null) {
                mOnPrepareListViewListener.onPrepareListView(listView);
            }
            dialog.mAdapter = adapter;
            dialog.mCheckedItem = mCheckedItem;
            if (mOnClickListener != null) {
                listView.setOnItemClickListener(new OnItemClickListener() {
                    @Override
                    public void onItemClick(AdapterView<?> parent, View v,
                            int position, long id) {
                        mOnClickListener.onClick(dialog.mDialogInterface,
                                position);
                        if (!mIsSingleChoice) {
                            dialog.mDialogInterface.dismiss();
                        }
                    }
                });
            } else if (mOnCheckboxClickListener != null) {
                listView.setOnItemClickListener(new OnItemClickListener() {
                    @Override
                    public void onItemClick(AdapterView<?> parent, View v,
                            int position, long id) {
                        if (mCheckedItems != null) {
                            mCheckedItems[position] = listView
                                    .isItemChecked(position);
                        }
                        mOnCheckboxClickListener.onClick(
                                dialog.mDialogInterface, position,
                                listView.isItemChecked(position));
                    }
                });
            }
            if (mOnItemSelectedListener != null) {
                listView.setOnItemSelectedListener(mOnItemSelectedListener);
            }
            if (mIsSingleChoice) {
                listView.setChoiceMode(AbsListView.CHOICE_MODE_SINGLE);
            } else if (mIsMultiChoice) {
                listView.setChoiceMode(AbsListView.CHOICE_MODE_MULTIPLE);
            }
            dialog.mListView = listView;
        }
    }

    private static final class ButtonHandler extends Handler {
        private static final int MSG_DISMISS_DIALOG = 1;
        private WeakReference<DialogInterface> mDialog;

        public ButtonHandler(DialogInterface dialog) {
            mDialog = new WeakReference<DialogInterface>(dialog);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case DialogInterface.BUTTON_POSITIVE:
                case DialogInterface.BUTTON_NEGATIVE:
                case DialogInterface.BUTTON_NEUTRAL:
                    ((DialogInterface.OnClickListener) msg.obj).onClick(
                            mDialog.get(), msg.what);
                    break;

                case MSG_DISMISS_DIALOG:
                    ((DialogInterface) msg.obj).dismiss();
            }
        }
    }

    static boolean canTextInput(View v) {
        if (v.onCheckIsTextEditor()) {
            return true;
        }
        if (!(v instanceof ViewGroup)) {
            return false;
        }
        ViewGroup vg = (ViewGroup) v;
        int i = vg.getChildCount();
        while (i > 0) {
            i--;
            v = vg.getChildAt(i);
            if (AlertController.canTextInput(v)) {
                return true;
            }
        }
        return false;
    }

    private static boolean shouldCenterSingleButton(Context context) {
        TypedValue outValue = new TypedValue();
        context.getTheme().resolveAttribute(R.attr.alertDialogCenterButtons,
                outValue, true);
        return outValue.data != 0;
    }

    private ListAdapter mAdapter;
    private int mAlertDialogLayout;
    private int mButtonBehavior = AlertDialog.DISMISS_ON_ALL;
    private View.OnClickListener mButtonHandler = new View.OnClickListener() {
        private boolean needToDismiss(int flag) {
            return (mButtonBehavior & flag) == flag;
        }

        @Override
        public void onClick(View v) {
            int button;
            if (v == mButtonPositive) {
                button = DialogInterface.BUTTON_POSITIVE;
            } else if (v == mButtonNegative) {
                button = DialogInterface.BUTTON_NEGATIVE;
            } else if (v == mButtonNeutral) {
                button = DialogInterface.BUTTON_NEUTRAL;
            } else {
                return;
            }
            boolean dismiss = false;
            switch (button) {
                case DialogInterface.BUTTON_POSITIVE:
                    send(mButtonPositiveMessage);
                    dismiss = needToDismiss(AlertDialog.DISMISS_ON_POSITIVE);
                    break;
                case DialogInterface.BUTTON_NEGATIVE:
                    send(mButtonNegativeMessage);
                    dismiss = needToDismiss(AlertDialog.DISMISS_ON_NEGATIVE);
                    break;
                case DialogInterface.BUTTON_NEUTRAL:
                    send(mButtonNeutralMessage);
                    dismiss = needToDismiss(AlertDialog.DISMISS_ON_NEUTRAL);
                    break;
            }
            if (dismiss) {
                mHandler.obtainMessage(ButtonHandler.MSG_DISMISS_DIALOG,
                        mDialogInterface).sendToTarget();
            }
        }

        private void send(Message m) {
            if (m == null) {
                return;
            }
            Message.obtain(m).sendToTarget();
        }
    };
    private Button mButtonNegative;
    private Message mButtonNegativeMessage;
    private CharSequence mButtonNegativeText;
    private Button mButtonNeutral;
    private Message mButtonNeutralMessage;
    private CharSequence mButtonNeutralText;
    private Button mButtonPositive;
    private Message mButtonPositiveMessage;
    private CharSequence mButtonPositiveText;
    private int mCheckedItem = -1;
    private final Context mContext;
    private View mCustomTitleView;
    private final AlertDecorViewInstaller mDecorViewInstaller;
    private final DialogInterface mDialogInterface;
    private boolean mForceInverseBackground;
    private Handler mHandler;
    private Drawable mIcon;
    private int mIconId = -1;
    private ImageView mIconView;
    private int mListItemLayout;
    private int mListLayout;
    private ListView mListView;
    private CharSequence mMessage;
    private TextView mMessageView;
    private int mMultiChoiceItemLayout;
    private ScrollView mScrollView;
    private int mSingleChoiceItemLayout;
    private CharSequence mTitle;
    private TextView mTitleView;
    private View mView;
    private int mViewSpacingBottom;
    private int mViewSpacingLeft;
    private int mViewSpacingRight;
    private boolean mViewSpacingSpecified = false;
    private int mViewSpacingTop;
    private final Window mWindow;

    public AlertController(Context context, DialogInterface di, Window window) {
        this(context, di, window, null);
    }

    public AlertController(Context context, DialogInterface di, Window window,
            AlertDecorViewInstaller decorViewInstaller) {
        mDecorViewInstaller = decorViewInstaller;
        mContext = context;
        mDialogInterface = di;
        mWindow = window;
        mHandler = new ButtonHandler(di);
        TypedArray a = context.obtainStyledAttributes(null,
                R.styleable.AlertDialog, R.attr.alertDialogStyle,
                R.style.Holo_AlertDialog);
        mAlertDialogLayout = a.getResourceId(R.styleable.AlertDialog_layout,
                R.layout.alert_dialog_holo);
        mListLayout = a.getResourceId(R.styleable.AlertDialog_listLayout,
                R.layout.select_dialog_holo);
        mMultiChoiceItemLayout = a.getResourceId(
                R.styleable.AlertDialog_multiChoiceItemLayout,
                R.layout.select_dialog_multichoice_holo);
        mSingleChoiceItemLayout = a.getResourceId(
                R.styleable.AlertDialog_singleChoiceItemLayout,
                R.layout.select_dialog_singlechoice_holo);
        mListItemLayout = a.getResourceId(
                R.styleable.AlertDialog_listItemLayout,
                R.layout.select_dialog_item_holo);
        a.recycle();
    }

    private void centerButton(Button button) {
        LinearLayout.LayoutParams params = (LinearLayout.LayoutParams) button
                .getLayoutParams();
        params.gravity = Gravity.CENTER_HORIZONTAL;
        params.weight = 0.5f;
        button.setLayoutParams(params);
        View leftSpacer = mWindow.findViewById(R.id.leftSpacer);
        if (leftSpacer != null) {
            leftSpacer.setVisibility(View.VISIBLE);
        }
        View rightSpacer = mWindow.findViewById(R.id.rightSpacer);
        if (rightSpacer != null) {
            rightSpacer.setVisibility(View.VISIBLE);
        }
    }

    public Button getButton(int whichButton) {
        switch (whichButton) {
            case DialogInterface.BUTTON_POSITIVE:
                return mButtonPositive;
            case DialogInterface.BUTTON_NEGATIVE:
                return mButtonNegative;
            case DialogInterface.BUTTON_NEUTRAL:
                return mButtonNeutral;
            default:
                return null;
        }
    }

    public ListView getListView() {
        return mListView;
    }

    public void installContent() {
        mWindow.requestFeature(Window.FEATURE_NO_TITLE);
        if (mView == null || !AlertController.canTextInput(mView)) {
            mWindow.setFlags(WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM,
                    WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM);
        }
        if (mDecorViewInstaller == null) {
            mWindow.setContentView(LayoutInflater.inflate(mWindow.getContext(),
                    mAlertDialogLayout));
        } else {
            mDecorViewInstaller.installDecorView(mContext, mAlertDialogLayout);
        }
        setupView();
    }

    public boolean onKeyDown(int keyCode, KeyEvent event) {
        return mScrollView != null && mScrollView.executeKeyEvent(event);
    }

    public boolean onKeyUp(int keyCode, KeyEvent event) {
        return mScrollView != null && mScrollView.executeKeyEvent(event);
    }

    private void setBackground(LinearLayout topPanel,
            LinearLayout contentPanel, View customPanel, boolean hasButtons,
            TypedArray a, boolean hasTitle, View buttonPanel) {
        int fullDark = a.getResourceId(R.styleable.AlertDialog_fullDark,
                R.drawable.dialog_full_holo_dark);
        int topDark = a.getResourceId(R.styleable.AlertDialog_topDark,
                R.drawable.dialog_top_holo_dark);
        int centerDark = a.getResourceId(R.styleable.AlertDialog_centerDark,
                R.drawable.dialog_middle_holo_dark);
        int bottomDark = a.getResourceId(R.styleable.AlertDialog_bottomDark,
                R.drawable.dialog_bottom_holo_dark);
        int fullBright = a.getResourceId(R.styleable.AlertDialog_fullBright,
                R.drawable.dialog_full_holo_dark);
        int topBright = a.getResourceId(R.styleable.AlertDialog_topBright,
                R.drawable.dialog_top_holo_dark);
        int centerBright = a.getResourceId(
                R.styleable.AlertDialog_centerBright,
                R.drawable.dialog_middle_holo_dark);
        int bottomBright = a.getResourceId(
                R.styleable.AlertDialog_bottomBright,
                R.drawable.dialog_bottom_holo_dark);
        int bottomMedium = a.getResourceId(
                R.styleable.AlertDialog_bottomMedium,
                R.drawable.dialog_bottom_holo_dark);
        View[] views = new View[4];
        boolean[] light = new boolean[4];
        View lastView = null;
        boolean lastLight = false;

        int pos = 0;
        if (hasTitle) {
            views[pos] = topPanel;
            light[pos] = false;
            pos++;
        }
        views[pos] = contentPanel.getVisibility() == View.GONE ? null
                : contentPanel;
        light[pos] = mListView != null;
        pos++;
        if (customPanel != null) {
            views[pos] = customPanel;
            light[pos] = mForceInverseBackground;
            pos++;
        }
        if (hasButtons) {
            views[pos] = buttonPanel;
            light[pos] = true;
        }
        boolean setView = false;
        for (pos = 0; pos < views.length; pos++) {
            View v = views[pos];
            if (v == null) {
                continue;
            }
            if (lastView != null) {
                if (!setView) {
                    lastView.setBackgroundResource(lastLight ? topBright
                            : topDark);
                } else {
                    lastView.setBackgroundResource(lastLight ? centerBright
                            : centerDark);
                }
                setView = true;
            }
            lastView = v;
            lastLight = light[pos];
        }
        if (lastView != null) {
            if (setView) {
                lastView.setBackgroundResource(lastLight ? hasButtons ? bottomMedium
                        : bottomBright
                        : bottomDark);
            } else {
                lastView.setBackgroundResource(lastLight ? fullBright
                        : fullDark);
            }
        }
        if (mListView != null && mAdapter != null) {
            mListView.setAdapter(mAdapter);
            if (mCheckedItem > -1) {
                mListView.setItemChecked(mCheckedItem, true);
                mListView.setSelection(mCheckedItem);
            }
        }
    }

    public void setButton(int whichButton, CharSequence text,
            DialogInterface.OnClickListener listener, Message msg) {
        if (msg == null && listener != null) {
            msg = mHandler.obtainMessage(whichButton, listener);
        }
        switch (whichButton) {
            case DialogInterface.BUTTON_POSITIVE:
                mButtonPositiveText = text;
                mButtonPositiveMessage = msg;
                break;
            case DialogInterface.BUTTON_NEGATIVE:
                mButtonNegativeText = text;
                mButtonNegativeMessage = msg;
                break;
            case DialogInterface.BUTTON_NEUTRAL:
                mButtonNeutralText = text;
                mButtonNeutralMessage = msg;
                break;
            default:
                throw new IllegalArgumentException("Button does not exist");
        }
    }

    public void setButtonBehavior(int buttonBehavior) {
        mButtonBehavior = buttonBehavior;
    }

    public void setCustomTitle(View customTitleView) {
        mCustomTitleView = customTitleView;
    }

    public void setIcon(Drawable icon) {
        mIcon = icon;
        if (mIconView != null && mIcon != null) {
            mIconView.setImageDrawable(icon);
        }
    }

    public void setIcon(int resId) {
        mIconId = resId;
        if (mIconView != null) {
            if (resId > 0) {
                mIconView.setImageResource(mIconId);
            } else if (resId == 0) {
                mIconView.setVisibility(View.GONE);
            }
        }
    }

    public void setInverseBackgroundForced(boolean forceInverseBackground) {
        mForceInverseBackground = forceInverseBackground;
    }

    public void setMessage(CharSequence message) {
        mMessage = message;
        if (mMessageView != null) {
            mMessageView.setText(message);
        }
    }

    public void setTitle(CharSequence title) {
        mTitle = title;
        if (mTitleView != null) {
            mTitleView.setText(title);
        }
    }

    private boolean setupButtons() {
        int BIT_BUTTON_POSITIVE = 1;
        int BIT_BUTTON_NEGATIVE = 2;
        int BIT_BUTTON_NEUTRAL = 4;
        int whichButtons = 0;
        mButtonPositive = (Button) mWindow.findViewById(R.id.button1);
        mButtonPositive.setOnClickListener(mButtonHandler);
        if (TextUtils.isEmpty(mButtonPositiveText)) {
            mButtonPositive.setVisibility(View.GONE);
        } else {
            mButtonPositive.setText(mButtonPositiveText);
            mButtonPositive.setVisibility(View.VISIBLE);
            whichButtons = whichButtons | BIT_BUTTON_POSITIVE;
        }
        mButtonNegative = (Button) mWindow.findViewById(R.id.button2);
        mButtonNegative.setOnClickListener(mButtonHandler);
        if (TextUtils.isEmpty(mButtonNegativeText)) {
            mButtonNegative.setVisibility(View.GONE);
        } else {
            mButtonNegative.setText(mButtonNegativeText);
            mButtonNegative.setVisibility(View.VISIBLE);

            whichButtons = whichButtons | BIT_BUTTON_NEGATIVE;
        }
        mButtonNeutral = (Button) mWindow.findViewById(R.id.button3);
        mButtonNeutral.setOnClickListener(mButtonHandler);
        if (TextUtils.isEmpty(mButtonNeutralText)) {
            mButtonNeutral.setVisibility(View.GONE);
        } else {
            mButtonNeutral.setText(mButtonNeutralText);
            mButtonNeutral.setVisibility(View.VISIBLE);

            whichButtons = whichButtons | BIT_BUTTON_NEUTRAL;
        }
        if (AlertController.shouldCenterSingleButton(mContext)) {
            if (whichButtons == BIT_BUTTON_POSITIVE) {
                centerButton(mButtonPositive);
            } else if (whichButtons == BIT_BUTTON_NEGATIVE) {
                centerButton(mButtonNeutral);
            } else if (whichButtons == BIT_BUTTON_NEUTRAL) {
                centerButton(mButtonNeutral);
            }
        }
        return whichButtons != 0;
    }

    private void setupContent(LinearLayout contentPanel) {
        mScrollView = (ScrollView) mWindow.findViewById(R.id.scrollView);
        mScrollView.setFocusable(false);
        mMessageView = (TextView) mWindow.findViewById(R.id.message);
        if (mMessageView == null) {
            return;
        }
        if (mMessage != null) {
            mMessageView.setText(mMessage);
        } else {
            mMessageView.setVisibility(View.GONE);
            mScrollView.removeView(mMessageView);
            if (mListView != null) {
                contentPanel.removeView(mWindow.findViewById(R.id.scrollView));
                contentPanel.addView(mListView, new LinearLayout.LayoutParams(
                        android.view.ViewGroup.LayoutParams.MATCH_PARENT,
                        android.view.ViewGroup.LayoutParams.MATCH_PARENT));
                contentPanel.setLayoutParams(new LinearLayout.LayoutParams(
                        android.view.ViewGroup.LayoutParams.MATCH_PARENT, 0,
                        1.0f));
            } else {
                contentPanel.setVisibility(View.GONE);
            }
        }
    }

    private boolean setupTitle(LinearLayout topPanel) {
        boolean hasTitle = true;
        if (mCustomTitleView != null) {
            LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT);

            topPanel.addView(mCustomTitleView, 0, lp);
            View titleTemplate = mWindow.findViewById(R.id.title_template);
            titleTemplate.setVisibility(View.GONE);
        } else {
            final boolean hasTextTitle = !TextUtils.isEmpty(mTitle);
            mIconView = (ImageView) mWindow.findViewById(R.id.icon);
            if (hasTextTitle) {
                mTitleView = (TextView) mWindow.findViewById(R.id.alertTitle);
                mTitleView.setText(mTitle);
                if (mIconId > 0) {
                    mIconView.setImageResource(mIconId);
                } else if (mIcon != null) {
                    mIconView.setImageDrawable(mIcon);
                } else if (mIconId == 0) {
                    mTitleView.setPadding(mIconView.getPaddingLeft(),
                            mIconView.getPaddingTop(),
                            mIconView.getPaddingRight(),
                            mIconView.getPaddingBottom());
                    mIconView.setVisibility(View.GONE);
                }
            } else {
                View titleTemplate = mWindow.findViewById(R.id.title_template);
                titleTemplate.setVisibility(View.GONE);
                mIconView.setVisibility(View.GONE);
                topPanel.setVisibility(View.GONE);
                hasTitle = false;
            }
        }
        return hasTitle;
    }

    private void setupView() {
        LinearLayout contentPanel = (LinearLayout) mWindow.findViewById(R.id.contentPanel);
        setupContent(contentPanel);
        boolean hasButtons = setupButtons();
        LinearLayout topPanel = (LinearLayout) mWindow.findViewById(R.id.topPanel);
        TypedArray a = mContext.obtainStyledAttributes(null,
                R.styleable.AlertDialog, R.attr.alertDialogStyle, R.style.Holo_AlertDialog);
        boolean hasTitle = setupTitle(topPanel);
        View buttonPanel = mWindow.findViewById(R.id.buttonPanel);
        if (!hasButtons) {
            buttonPanel.setVisibility(View.GONE);
            // mWindow.setCloseOnTouchOutsideIfNotSet(true);
        }
        FrameLayout customPanel = null;
        if (mView != null) {
            customPanel = (FrameLayout) mWindow.findViewById(R.id.customPanel);
            FrameLayout custom = (FrameLayout) mWindow.findViewById(R.id.custom);
            custom.addView(mView, new LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.MATCH_PARENT));
            if (mViewSpacingSpecified) {
                custom.setPadding(mViewSpacingLeft, mViewSpacingTop,
                        mViewSpacingRight, mViewSpacingBottom);
            }
            if (mListView != null) {
                ((LinearLayout.LayoutParams) customPanel.getLayoutParams()).weight = 0;
            }
        } else {
            mWindow.findViewById(R.id.customPanel).setVisibility(View.GONE);
        }

        if (hasTitle) {
            View divider = null;
            if (mMessage != null || mView != null || mListView != null) {
                divider = mWindow.findViewById(R.id.titleDivider);
            } else {
                divider = mWindow.findViewById(R.id.titleDividerTop);
            }

            if (divider != null) {
                divider.setVisibility(View.VISIBLE);
            }
        }
        setBackground(topPanel, contentPanel, customPanel, hasButtons, a,
                hasTitle, buttonPanel);
        a.recycle();
    }

    public void setView(View view) {
        mView = view;
        mViewSpacingSpecified = false;
    }

    public void setView(View view, int viewSpacingLeft, int viewSpacingTop,
            int viewSpacingRight, int viewSpacingBottom) {
        mView = view;
        mViewSpacingSpecified = true;
        mViewSpacingLeft = viewSpacingLeft;
        mViewSpacingTop = viewSpacingTop;
        mViewSpacingRight = viewSpacingRight;
        mViewSpacingBottom = viewSpacingBottom;
    }
}
