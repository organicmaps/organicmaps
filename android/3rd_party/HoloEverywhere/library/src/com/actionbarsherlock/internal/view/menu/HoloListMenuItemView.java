
package com.actionbarsherlock.internal.view.menu;

import org.holoeverywhere.LayoutInflater;
import org.holoeverywhere.R;
import org.holoeverywhere.widget.LinearLayout;
import org.holoeverywhere.widget.TextView;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.ImageView;
import android.widget.RadioButton;

public class HoloListMenuItemView extends LinearLayout implements
        MenuView.ItemView {
    private Drawable mBackground;
    private CheckBox mCheckBox;
    private boolean mForceShowIcon;
    private ImageView mIconView;
    private LayoutInflater mInflater;
    private MenuItemImpl mItemData;
    private boolean mPreserveIconSpacing;
    private RadioButton mRadioButton;
    private TextView mShortcutView;
    private int mTextAppearance;
    private Context mTextAppearanceContext;
    private TextView mTitleView;

    public HoloListMenuItemView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public HoloListMenuItemView(Context context, AttributeSet attrs,
            int defStyle) {
        super(context, attrs);
        TypedArray a = context.obtainStyledAttributes(attrs,
                R.styleable.MenuView, defStyle, 0);
        mBackground = a.getDrawable(R.styleable.MenuView_dialogItemBackground);
        mTextAppearance = a.getResourceId(
                R.styleable.MenuView_dialogItemTextAppearance, -1);
        mPreserveIconSpacing = a.getBoolean(
                R.styleable.MenuView_android_preserveIconSpacing, false);
        mTextAppearanceContext = context;
        a.recycle();
    }

    private LayoutInflater getInflater() {
        if (mInflater == null) {
            mInflater = LayoutInflater.from(getContext());
        }
        return mInflater;
    }

    @Override
    public MenuItemImpl getItemData() {
        return mItemData;
    }

    @Override
    public void initialize(MenuItemImpl itemData, int menuType) {
        mItemData = itemData;
        setVisibility(itemData.isVisible() ? View.VISIBLE : View.GONE);
        setTitle(itemData.getTitleForItemView(this));
        setCheckable(itemData.isCheckable());
        setShortcut(itemData.shouldShowShortcut(), itemData.getShortcut());
        setIcon(itemData.getIcon());
        setEnabled(itemData.isEnabled());
    }

    private void insertCheckBox() {
        LayoutInflater inflater = getInflater();
        mCheckBox = (CheckBox) inflater.inflate(
                R.layout.list_menu_item_checkbox, this, false);
        addView(mCheckBox);
    }

    private void insertIconView() {
        LayoutInflater inflater = getInflater();
        mIconView = (ImageView) inflater.inflate(R.layout.list_menu_item_icon,
                this, false);
        addView(mIconView, 0);
    }

    private void insertRadioButton() {
        LayoutInflater inflater = getInflater();
        mRadioButton = (RadioButton) inflater.inflate(
                R.layout.list_menu_item_radio, this, false);
        addView(mRadioButton);
    }

    @SuppressWarnings("deprecation")
    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        setBackgroundDrawable(mBackground);
        mTitleView = (TextView) findViewById(R.id.title);
        if (mTextAppearance != -1) {
            mTitleView.setTextAppearance(mTextAppearanceContext,
                    mTextAppearance);
        }
        mShortcutView = (TextView) findViewById(R.id.shortcut);
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        if (mIconView != null && mPreserveIconSpacing) {
            // Enforce minimum icon spacing
            ViewGroup.LayoutParams lp = getLayoutParams();
            LayoutParams iconLp = (LayoutParams) mIconView.getLayoutParams();
            if (lp.height > 0 && iconLp.width <= 0) {
                iconLp.width = lp.height;
            }
        }
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }

    @Override
    public boolean prefersCondensedTitle() {
        return false;
    }

    @Override
    public void setCheckable(boolean checkable) {
        if (!checkable && mRadioButton == null && mCheckBox == null) {
            return;
        }

        // Depending on whether its exclusive check or not, the checkbox or
        // radio button will be the one in use (and the other will be
        // otherCompoundButton)
        final CompoundButton compoundButton;
        final CompoundButton otherCompoundButton;

        if (mItemData.isExclusiveCheckable()) {
            if (mRadioButton == null) {
                insertRadioButton();
            }
            compoundButton = mRadioButton;
            otherCompoundButton = mCheckBox;
        } else {
            if (mCheckBox == null) {
                insertCheckBox();
            }
            compoundButton = mCheckBox;
            otherCompoundButton = mRadioButton;
        }

        if (checkable) {
            compoundButton.setChecked(mItemData.isChecked());

            final int newVisibility = checkable ? View.VISIBLE : View.GONE;
            if (compoundButton.getVisibility() != newVisibility) {
                compoundButton.setVisibility(newVisibility);
            }

            // Make sure the other compound button isn't visible
            if (otherCompoundButton != null
                    && otherCompoundButton.getVisibility() != View.GONE) {
                otherCompoundButton.setVisibility(View.GONE);
            }
        } else {
            if (mCheckBox != null) {
                mCheckBox.setVisibility(View.GONE);
            }
            if (mRadioButton != null) {
                mRadioButton.setVisibility(View.GONE);
            }
        }
    }

    @Override
    public void setChecked(boolean checked) {
        CompoundButton compoundButton;

        if (mItemData.isExclusiveCheckable()) {
            if (mRadioButton == null) {
                insertRadioButton();
            }
            compoundButton = mRadioButton;
        } else {
            if (mCheckBox == null) {
                insertCheckBox();
            }
            compoundButton = mCheckBox;
        }

        compoundButton.setChecked(checked);
    }

    public void setForceShowIcon(boolean forceShow) {
        mPreserveIconSpacing = mForceShowIcon = forceShow;
    }

    @Override
    public void setIcon(Drawable icon) {
        final boolean showIcon = mItemData.shouldShowIcon() || mForceShowIcon;
        if (!showIcon && !mPreserveIconSpacing) {
            return;
        }

        if (mIconView == null && icon == null && !mPreserveIconSpacing) {
            return;
        }

        if (mIconView == null) {
            insertIconView();
        }

        if (icon != null || mPreserveIconSpacing) {
            mIconView.setImageDrawable(showIcon ? icon : null);

            if (mIconView.getVisibility() != View.VISIBLE) {
                mIconView.setVisibility(View.VISIBLE);
            }
        } else {
            mIconView.setVisibility(View.GONE);
        }
    }

    @Override
    public void setShortcut(boolean showShortcut, char shortcutKey) {
        final int newVisibility = showShortcut
                && mItemData.shouldShowShortcut() ? View.VISIBLE : View.GONE;

        if (newVisibility == View.VISIBLE) {
            mShortcutView.setText(mItemData.getShortcutLabel());
        }

        if (mShortcutView.getVisibility() != newVisibility) {
            mShortcutView.setVisibility(newVisibility);
        }
    }

    @Override
    public void setTitle(CharSequence title) {
        if (title != null) {
            mTitleView.setText(title);

            if (mTitleView.getVisibility() != View.VISIBLE) {
                mTitleView.setVisibility(View.VISIBLE);
            }
        } else {
            if (mTitleView.getVisibility() != View.GONE) {
                mTitleView.setVisibility(View.GONE);
            }
        }
    }

    @Override
    public boolean showsIcon() {
        return mForceShowIcon;
    }
}
