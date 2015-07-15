package com.cocosw.bottomsheet;


/*
 * Copyright 2013 Hari Krishna Dulipudi
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


        import android.content.Context;
        import android.database.DataSetObserver;
        import android.graphics.Canvas;
        import android.graphics.Color;
        import android.graphics.PointF;
        import android.graphics.Rect;
        import android.graphics.drawable.GradientDrawable;
        import android.graphics.drawable.GradientDrawable.Orientation;
        import android.os.Parcelable;
        import android.util.AttributeSet;
        import android.view.MotionEvent;
        import android.view.SoundEffectConstants;
        import android.view.View;
        import android.view.ViewConfiguration;
        import android.view.accessibility.AccessibilityEvent;
        import android.widget.AbsListView;
        import android.widget.GridView;

/**
 * ListView capable to pin views at its top while the rest is still scrolled.
 */
 class PinnedSectionGridView extends GridView {


    // -- class fields

    private int mNumColumns;
    private int mHorizontalSpacing;
    private int mColumnWidth;
    private int mAvailableWidth;

    public PinnedSectionGridView(Context context) {
        super(context);
    }

    public PinnedSectionGridView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public PinnedSectionGridView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }


    @Override
    public void setNumColumns(int numColumns) {
        mNumColumns = numColumns;
        super.setNumColumns(numColumns);
    }

    public int getNumColumns(){
        return mNumColumns;
    }

    @Override
    public void setHorizontalSpacing(int horizontalSpacing) {
        mHorizontalSpacing = horizontalSpacing;
        super.setHorizontalSpacing(horizontalSpacing);
    }

    public int getHorizontalSpacing(){
        return mHorizontalSpacing;
    }

    @Override
    public void setColumnWidth(int columnWidth) {
        mColumnWidth = columnWidth;
        super.setColumnWidth(columnWidth);
    }

    public int getColumnWidth(){
        return mColumnWidth;
    }

    public int getAvailableWidth(){
        return mAvailableWidth != 0 ? mAvailableWidth : getWidth();
    }

//    @Override
//    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
//        if (mNumColumns == GridView.AUTO_FIT) {
//            mAvailableWidth = MeasureSpec.getSize(widthMeasureSpec);
//            if (mColumnWidth > 0) {
//                int availableSpace = MeasureSpec.getSize(widthMeasureSpec) - getPaddingLeft() - getPaddingRight();
//                // Client told us to pick the number of columns
//                mNumColumns = (availableSpace + mHorizontalSpacing) /
//                        (mColumnWidth + mHorizontalSpacing);
//            } else {
//                // Just make up a number if we don't have enough info
//                mNumColumns = 2;
//            }
//            if(null != getAdapter()){
//                if(getAdapter() instanceof SimpleSectionedGridAdapter){
//                    ((SimpleSectionedGridAdapter)getAdapter()).setSections();
//                }
//            }
//        }
//        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
//    }
}