package com.cocosw.bottomsheet;



import android.content.Context;
import android.util.AttributeSet;
import android.widget.FrameLayout;


class HeaderLayout extends FrameLayout {
    private int mHeaderWidth = 1;


    public HeaderLayout(Context context) {
        super(context);
    }


    public HeaderLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
    }


    public HeaderLayout(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    public void setHeaderWidth(int width) {
        mHeaderWidth = width;
    }


    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int widthMeasureSpecNew = mHeaderWidth == 1
                ? widthMeasureSpec
                : MeasureSpec.makeMeasureSpec(mHeaderWidth, MeasureSpec.getMode(widthMeasureSpec));
        super.onMeasure(widthMeasureSpecNew, heightMeasureSpec);
    }
}