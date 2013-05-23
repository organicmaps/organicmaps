
package com.actionbarsherlock.internal.view.menu;

import org.holoeverywhere.widget.ListView;

import android.content.Context;
import android.util.AttributeSet;
import android.util.TypedValue;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;

import com.actionbarsherlock.internal.view.menu.MenuBuilder.ItemInvoker;

public final class ExpandedMenuView extends ListView implements ItemInvoker,
        MenuView, OnItemClickListener {
    private int mAnimations;
    private MenuBuilder mMenu;

    public ExpandedMenuView(Context context, AttributeSet attrs) {
        super(context, attrs);
        TypedValue value = new TypedValue();
        context.getTheme().resolveAttribute(
                android.R.attr.windowAnimationStyle, value, true);
        mAnimations = value.resourceId;
        setOnItemClickListener(this);
    }

    @Override
    public int getWindowAnimations() {
        return mAnimations;
    }

    @Override
    public void initialize(MenuBuilder menu) {
        mMenu = menu;
    }

    @Override
    public boolean invokeItem(MenuItemImpl item) {
        return mMenu.performItemAction(item, 0);
    }

    @Override
    protected void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        setChildrenDrawingCacheEnabled(false);
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View v, int position, long id) {
        invokeItem((MenuItemImpl) getAdapter().getItem(position));
    }
}
