
package org.holoeverywhere.widget;

import org.holoeverywhere.R;

import android.content.Context;
import android.content.res.ColorStateList;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.NinePatchDrawable;
import android.os.Handler;
import android.os.SystemClock;
import android.support.v4.view.MotionEventCompat;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewConfiguration;
import android.widget.AbsListView;
import android.widget.AbsListView.OnScrollListener;
import android.widget.ExpandableListAdapter;
import android.widget.ListAdapter;
import android.widget.SectionIndexer;
import android.widget.WrapperListAdapter;

class FastScroller {
    public class ScrollFade implements Runnable {
        static final int ALPHA_MAX = 208;
        static final long FADE_DURATION = 200;
        long mFadeDuration;
        long mStartTime;

        int getAlpha() {
            if (getState() != STATE_EXIT) {
                return ALPHA_MAX;
            }
            int alpha;
            long now = SystemClock.uptimeMillis();
            if (now > mStartTime + mFadeDuration) {
                alpha = 0;
            } else {
                alpha = (int) (ALPHA_MAX - (now - mStartTime) * ALPHA_MAX / mFadeDuration);
            }
            return alpha;
        }

        @Override
        public void run() {
            if (getState() != STATE_EXIT) {
                startFade();
                return;
            }
            if (getAlpha() > 0) {
                mList.invalidate();
            } else {
                setState(STATE_NONE);
            }
        }

        void startFade() {
            mFadeDuration = FADE_DURATION;
            mStartTime = SystemClock.uptimeMillis();
            setState(STATE_EXIT);
        }
    }

    private static final int[] DEFAULT_STATES = new int[0];
    private static final int FADE_TIMEOUT = 1500;
    private static int MIN_PAGES = 4;
    private static final int OVERLAY_AT_THUMB = 1;
    private static final int OVERLAY_FLOATING = 0;
    private static final int PENDING_DRAG_DELAY = 180;
    private static final int[] PRESSED_STATES = new int[] {
            android.R.attr.state_pressed
    };
    private static final int STATE_DRAGGING = 2;
    private static final int STATE_EXIT = 3;
    private static final int STATE_NONE = 0;
    private static final int STATE_VISIBLE = 1;
    private boolean mAlwaysShow;
    private boolean mChangedBounds;
    private final Runnable mDeferStartDrag = new Runnable() {
        @Override
        public void run() {
            if (mList.isAttached()) {
                beginDrag();
                final int viewHeight = mList.getHeight();
                int newThumbY = (int) mInitialTouchY - mThumbH + 10;
                if (newThumbY < 0) {
                    newThumbY = 0;
                } else if (newThumbY + mThumbH > viewHeight) {
                    newThumbY = viewHeight - mThumbH;
                }
                mThumbY = newThumbY;
                scrollTo((float) mThumbY / (viewHeight - mThumbH));
            }
            mPendingDrag = false;
        }
    };
    private boolean mDrawOverlay;
    private Handler mHandler = new Handler();
    float mInitialTouchY;
    private int mItemCount = -1;
    ListView mList;
    ListAdapter mListAdapter;
    private int mListOffset;
    private boolean mLongList;
    private boolean mMatchDragPosition;
    private Drawable mOverlayDrawable;
    private Drawable mOverlayDrawableLeft;
    private Drawable mOverlayDrawableRight;
    private RectF mOverlayPos;
    private int mOverlayPosition;
    private int mOverlaySize;
    private Paint mPaint;
    boolean mPendingDrag;
    private int mPosition;
    private int mScaledTouchSlop;
    boolean mScrollCompleted;
    private ScrollFade mScrollFade;
    private SectionIndexer mSectionIndexer;
    private Object[] mSections;
    private String mSectionText;
    private int mState;
    private Drawable mThumbDrawable;
    int mThumbH;
    int mThumbW;
    int mThumbY;
    private final Rect mTmpRect = new Rect();
    private Drawable mTrackDrawable;
    private int mVisibleItem;

    public FastScroller(Context context, ListView listView) {
        mList = listView;
        init(context);
    }

    void beginDrag() {
        setState(STATE_DRAGGING);
        if (mListAdapter == null && mList != null) {
            getSectionsFromIndexer();
        }
        if (mList != null) {
            mList.requestDisallowInterceptTouchEvent(true);
            mList.reportScrollStateChange(OnScrollListener.SCROLL_STATE_TOUCH_SCROLL);
        }
        cancelFling();
    }

    private void cancelFling() {
        MotionEvent cancelFling = MotionEvent.obtain(0, 0, MotionEvent.ACTION_CANCEL, 0, 0, 0);
        mList.onTouchEvent(cancelFling);
        cancelFling.recycle();
    }

    void cancelPendingDrag() {
        mList.removeCallbacks(mDeferStartDrag);
        mPendingDrag = false;
    }

    public void draw(Canvas canvas) {
        if (mState == STATE_NONE) {
            return;
        }
        final int y = mThumbY;
        final int viewWidth = mList.getWidth();
        final FastScroller.ScrollFade scrollFade = mScrollFade;
        int alpha = -1;
        if (mState == STATE_EXIT) {
            alpha = scrollFade.getAlpha();
            if (alpha < ScrollFade.ALPHA_MAX / 2) {
                mThumbDrawable.setAlpha(alpha * 2);
            }
            int left = 0;
            switch (mPosition) {
                case View.SCROLLBAR_POSITION_DEFAULT:
                case View.SCROLLBAR_POSITION_RIGHT:
                    left = viewWidth - mThumbW * alpha / ScrollFade.ALPHA_MAX;
                    break;
                case View.SCROLLBAR_POSITION_LEFT:
                    left = -mThumbW + mThumbW * alpha / ScrollFade.ALPHA_MAX;
                    break;
            }
            mThumbDrawable.setBounds(left, 0, left + mThumbW, mThumbH);
            mChangedBounds = true;
        }
        if (mTrackDrawable != null) {
            final Rect thumbBounds = mThumbDrawable.getBounds();
            final int left = thumbBounds.left;
            final int halfThumbHeight = (thumbBounds.bottom - thumbBounds.top) / 2;
            final int trackWidth = mTrackDrawable.getIntrinsicWidth();
            final int trackLeft = left + mThumbW / 2 - trackWidth / 2;
            mTrackDrawable.setBounds(trackLeft, halfThumbHeight,
                    trackLeft + trackWidth, mList.getHeight() - halfThumbHeight);
            mTrackDrawable.draw(canvas);
        }
        canvas.translate(0, y);
        mThumbDrawable.draw(canvas);
        canvas.translate(0, -y);
        if (mState == STATE_DRAGGING && mDrawOverlay) {
            if (mOverlayPosition == OVERLAY_AT_THUMB) {
                int left = 0;
                switch (mPosition) {
                    default:
                    case View.SCROLLBAR_POSITION_DEFAULT:
                    case View.SCROLLBAR_POSITION_RIGHT:
                        left = Math.max(0,
                                mThumbDrawable.getBounds().left - mThumbW - mOverlaySize);
                        break;
                    case View.SCROLLBAR_POSITION_LEFT:
                        left = Math.min(mThumbDrawable.getBounds().right + mThumbW,
                                mList.getWidth() - mOverlaySize);
                        break;
                }
                int top = Math
                        .max(0,
                                Math.min(y + (mThumbH - mOverlaySize) / 2, mList.getHeight()
                                        - mOverlaySize));
                final RectF pos = mOverlayPos;
                pos.left = left;
                pos.right = pos.left + mOverlaySize;
                pos.top = top;
                pos.bottom = pos.top + mOverlaySize;
                if (mOverlayDrawable != null) {
                    mOverlayDrawable.setBounds((int) pos.left, (int) pos.top,
                            (int) pos.right, (int) pos.bottom);
                }
            }
            mOverlayDrawable.draw(canvas);
            final Paint paint = mPaint;
            float descent = paint.descent();
            final RectF rectF = mOverlayPos;
            final Rect tmpRect = mTmpRect;
            mOverlayDrawable.getPadding(tmpRect);
            final int hOff = (tmpRect.right - tmpRect.left) / 2;
            final int vOff = (tmpRect.bottom - tmpRect.top) / 2;
            canvas.drawText(mSectionText, (int) (rectF.left + rectF.right) / 2 - hOff,
                    (int) (rectF.bottom + rectF.top) / 2 + mOverlaySize / 4 - descent - vOff,
                    paint);
        } else if (mState == STATE_EXIT) {
            if (alpha == 0) {
                setState(STATE_NONE);
            } else if (mTrackDrawable != null) {
                mList.invalidate(viewWidth - mThumbW, 0, viewWidth, mList.getHeight());
            } else {
                mList.invalidate(viewWidth - mThumbW, y, viewWidth, y + mThumbH);
            }
        }
    }

    SectionIndexer getSectionIndexer() {
        return mSectionIndexer;
    }

    Object[] getSections() {
        if (mListAdapter == null && mList != null) {
            getSectionsFromIndexer();
        }
        return mSections;
    }

    void getSectionsFromIndexer() {
        ListAdapter adapter = mList.getAdapter();
        mSectionIndexer = null;
        if (adapter instanceof HeaderViewListAdapter) {
            mListOffset = ((HeaderViewListAdapter) adapter).getHeadersCount();
        }
        if (adapter instanceof ListAdapterWrapper) {
            adapter = ((ListAdapterWrapper) adapter).getWrappedAdapter();
        }
        if (adapter instanceof WrapperListAdapter) {
            adapter = ((WrapperListAdapter) adapter).getWrappedAdapter();
        }
        if (adapter instanceof ExpandableListConnector) {
            ExpandableListAdapter expAdapter = ((ExpandableListConnector) adapter).getAdapter();
            if (expAdapter instanceof SectionIndexer) {
                mSectionIndexer = (SectionIndexer) expAdapter;
                mListAdapter = adapter;
                mSections = mSectionIndexer.getSections();
            }
        } else {
            if (adapter instanceof SectionIndexer) {
                mSectionIndexer = (SectionIndexer) adapter;
                mSections = mSectionIndexer.getSections();
                if (mSections == null) {
                    mSections = new String[] {
                            " "
                    };
                }
            } else {
                mSections = new String[] {
                        " "
                };
            }
        }
        mListAdapter = adapter;
    }

    public int getState() {
        return mState;
    }

    private int getThumbPositionForListPosition(int firstVisibleItem, int visibleItemCount,
            int totalItemCount) {
        if (mSectionIndexer == null || mListAdapter == null) {
            getSectionsFromIndexer();
        }
        if (mSectionIndexer == null || !mMatchDragPosition) {
            return (mList.getHeight() - mThumbH) * firstVisibleItem
                    / (totalItemCount - visibleItemCount);
        }
        firstVisibleItem -= mListOffset;
        if (firstVisibleItem < 0) {
            return 0;
        }
        totalItemCount -= mListOffset;
        final int trackHeight = mList.getHeight() - mThumbH;
        final int section = mSectionIndexer.getSectionForPosition(firstVisibleItem);
        final int sectionPos = mSectionIndexer.getPositionForSection(section);
        final int nextSectionPos = mSectionIndexer.getPositionForSection(section + 1);
        final int sectionCount = mSections.length;
        final int positionsInSection = nextSectionPos - sectionPos;
        final View child = mList.getChildAt(0);
        final float incrementalPos = child == null ? 0 : firstVisibleItem +
                (float) (mList.getPaddingTop() - child.getTop()) / child.getHeight();
        final float posWithinSection = (incrementalPos - sectionPos) / positionsInSection;
        int result = (int) ((section + posWithinSection) / sectionCount * trackHeight);
        if (firstVisibleItem > 0 && firstVisibleItem + visibleItemCount == totalItemCount) {
            final View lastChild = mList.getChildAt(visibleItemCount - 1);
            final float lastItemVisible = (float) (mList.getHeight() - mList.getPaddingBottom()
                    - lastChild.getTop()) / lastChild.getHeight();
            result += (trackHeight - result) * lastItemVisible;
        }
        return result;
    }

    public int getWidth() {
        return mThumbW;
    }

    private void init(Context context) {
        TypedArray ta = context.getTheme().obtainStyledAttributes(R.styleable.FastScroll);
        useThumbDrawable(context, ta.getDrawable(R.styleable.FastScroll_fastScrollThumbDrawable));
        mTrackDrawable = ta.getDrawable(R.styleable.FastScroll_fastScrollTrackDrawable);
        mOverlayDrawableLeft = ta
                .getDrawable(R.styleable.FastScroll_fastScrollPreviewBackgroundLeft);
        mOverlayDrawableRight = ta
                .getDrawable(R.styleable.FastScroll_fastScrollPreviewBackgroundRight);
        mOverlayPosition = ta.getInt(R.styleable.FastScroll_fastScrollOverlayPosition,
                OVERLAY_FLOATING);
        mScrollCompleted = true;
        getSectionsFromIndexer();
        mOverlaySize = context.getResources().getDimensionPixelSize(
                R.dimen.fastscroll_overlay_size);
        mOverlayPos = new RectF();
        mScrollFade = new ScrollFade();
        mPaint = new Paint();
        mPaint.setAntiAlias(true);
        mPaint.setTextAlign(Paint.Align.CENTER);
        mPaint.setTextSize(mOverlaySize / 2);
        ColorStateList textColor = ta.getColorStateList(R.styleable.FastScroll_fastScrollTextColor);
        int textColorNormal = textColor.getDefaultColor();
        mPaint.setColor(textColorNormal);
        mPaint.setStyle(Paint.Style.FILL_AND_STROKE);
        if (mList.getWidth() > 0 && mList.getHeight() > 0) {
            onSizeChanged(mList.getWidth(), mList.getHeight(), 0, 0);
        }
        mState = STATE_NONE;
        refreshDrawableState();
        ta.recycle();
        mScaledTouchSlop = ViewConfiguration.get(context).getScaledTouchSlop();
        mMatchDragPosition = context.getApplicationInfo().targetSdkVersion >=
                android.os.Build.VERSION_CODES.HONEYCOMB;
        setScrollbarPosition(mList.getVerticalScrollbarPosition());
    }

    public boolean isAlwaysShowEnabled() {
        return mAlwaysShow;
    }

    boolean isPointInside(float x, float y) {
        boolean inTrack = false;
        switch (mPosition) {
            default:
            case View.SCROLLBAR_POSITION_DEFAULT:
            case View.SCROLLBAR_POSITION_RIGHT:
                inTrack = x > mList.getWidth() - mThumbW;
                break;
            case View.SCROLLBAR_POSITION_LEFT:
                inTrack = x < mThumbW;
                break;
        }
        return inTrack && (mTrackDrawable != null || y >= mThumbY && y <= mThumbY + mThumbH);
    }

    boolean isVisible() {
        return !(mState == STATE_NONE);
    }

    boolean onInterceptTouchEvent(MotionEvent ev) {
        switch (MotionEventCompat.getActionMasked(ev)) {
            case MotionEvent.ACTION_DOWN:
                if (mState > STATE_NONE && isPointInside(ev.getX(), ev.getY())) {
                    if (!mList.isInScrollingContainer()) {
                        beginDrag();
                        return true;
                    }
                    mInitialTouchY = ev.getY();
                    startPendingDrag();
                }
                break;
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_CANCEL:
                cancelPendingDrag();
                break;
        }
        return false;
    }

    void onItemCountChanged(int oldCount, int newCount) {
        if (mAlwaysShow) {
            mLongList = true;
        }
    }

    void onScroll(AbsListView view, int firstVisibleItem, int visibleItemCount,
            int totalItemCount) {
        if (mItemCount != totalItemCount && visibleItemCount > 0) {
            mItemCount = totalItemCount;
            mLongList = mItemCount / visibleItemCount >= MIN_PAGES;
        }
        if (mAlwaysShow) {
            mLongList = true;
        }
        if (!mLongList) {
            if (mState != STATE_NONE) {
                setState(STATE_NONE);
            }
            return;
        }
        if (totalItemCount - visibleItemCount > 0 && mState != STATE_DRAGGING) {
            mThumbY = getThumbPositionForListPosition(firstVisibleItem, visibleItemCount,
                    totalItemCount);
            if (mChangedBounds) {
                resetThumbPos();
                mChangedBounds = false;
            }
        }
        mScrollCompleted = true;
        if (firstVisibleItem == mVisibleItem) {
            return;
        }
        mVisibleItem = firstVisibleItem;
        if (mState != STATE_DRAGGING) {
            setState(STATE_VISIBLE);
            if (!mAlwaysShow) {
                mHandler.postDelayed(mScrollFade, FADE_TIMEOUT);
            }
        }
    }

    public void onSectionsChanged() {
        mListAdapter = null;
    }

    void onSizeChanged(int w, int h, int oldw, int oldh) {
        if (mThumbDrawable != null) {
            switch (mPosition) {
                default:
                case View.SCROLLBAR_POSITION_DEFAULT:
                case View.SCROLLBAR_POSITION_RIGHT:
                    mThumbDrawable.setBounds(w - mThumbW, 0, w, mThumbH);
                    break;
                case View.SCROLLBAR_POSITION_LEFT:
                    mThumbDrawable.setBounds(0, 0, mThumbW, mThumbH);
                    break;
            }
        }
        if (mOverlayPosition == OVERLAY_FLOATING) {
            final RectF pos = mOverlayPos;
            pos.left = (w - mOverlaySize) / 2;
            pos.right = pos.left + mOverlaySize;
            pos.top = h / 10;
            pos.bottom = pos.top + mOverlaySize;
            if (mOverlayDrawable != null) {
                mOverlayDrawable.setBounds((int) pos.left, (int) pos.top,
                        (int) pos.right, (int) pos.bottom);
            }
        }
    }

    boolean onTouchEvent(MotionEvent me) {
        if (mState == STATE_NONE) {
            return false;
        }
        final int action = me.getAction();
        if (action == MotionEvent.ACTION_DOWN) {
            if (isPointInside(me.getX(), me.getY())) {
                if (!mList.isInScrollingContainer()) {
                    beginDrag();
                    return true;
                }
                mInitialTouchY = me.getY();
                startPendingDrag();
            }
        } else if (action == MotionEvent.ACTION_UP) {
            if (mPendingDrag) {
                beginDrag();
                final int viewHeight = mList.getHeight();
                int newThumbY = (int) me.getY() - mThumbH + 10;
                if (newThumbY < 0) {
                    newThumbY = 0;
                } else if (newThumbY + mThumbH > viewHeight) {
                    newThumbY = viewHeight - mThumbH;
                }
                mThumbY = newThumbY;
                scrollTo((float) mThumbY / (viewHeight - mThumbH));
                cancelPendingDrag();
            }
            if (mState == STATE_DRAGGING) {
                if (mList != null) {
                    mList.requestDisallowInterceptTouchEvent(false);
                    mList.reportScrollStateChange(OnScrollListener.SCROLL_STATE_IDLE);
                }
                setState(STATE_VISIBLE);
                final Handler handler = mHandler;
                handler.removeCallbacks(mScrollFade);
                if (!mAlwaysShow) {
                    handler.postDelayed(mScrollFade, 1000);
                }
                mList.invalidate();
                return true;
            }
        } else if (action == MotionEvent.ACTION_MOVE) {
            if (mPendingDrag) {
                final float y = me.getY();
                if (Math.abs(y - mInitialTouchY) > mScaledTouchSlop) {
                    setState(STATE_DRAGGING);
                    if (mListAdapter == null && mList != null) {
                        getSectionsFromIndexer();
                    }
                    if (mList != null) {
                        mList.requestDisallowInterceptTouchEvent(true);
                        mList.reportScrollStateChange(OnScrollListener.SCROLL_STATE_TOUCH_SCROLL);
                    }
                    cancelFling();
                    cancelPendingDrag();
                }
            }
            if (mState == STATE_DRAGGING) {
                final int viewHeight = mList.getHeight();
                int newThumbY = (int) me.getY() - mThumbH + 10;
                if (newThumbY < 0) {
                    newThumbY = 0;
                } else if (newThumbY + mThumbH > viewHeight) {
                    newThumbY = viewHeight - mThumbH;
                }
                if (Math.abs(mThumbY - newThumbY) < 2) {
                    return true;
                }
                mThumbY = newThumbY;
                if (mScrollCompleted) {
                    scrollTo((float) mThumbY / (viewHeight - mThumbH));
                }
                return true;
            }
        } else if (action == MotionEvent.ACTION_CANCEL) {
            cancelPendingDrag();
        }
        return false;
    }

    private void refreshDrawableState() {
        int[] state = mState == STATE_DRAGGING ? PRESSED_STATES : DEFAULT_STATES;
        if (mThumbDrawable != null && mThumbDrawable.isStateful()) {
            mThumbDrawable.setState(state);
        }
        if (mTrackDrawable != null && mTrackDrawable.isStateful()) {
            mTrackDrawable.setState(state);
        }
    }

    private void resetThumbPos() {
        final int viewWidth = mList.getWidth();
        switch (mPosition) {
            case View.SCROLLBAR_POSITION_DEFAULT:
            case View.SCROLLBAR_POSITION_RIGHT:
                mThumbDrawable.setBounds(viewWidth - mThumbW, 0, viewWidth, mThumbH);
                break;
            case View.SCROLLBAR_POSITION_LEFT:
                mThumbDrawable.setBounds(0, 0, mThumbW, mThumbH);
                break;
        }
        mThumbDrawable.setAlpha(ScrollFade.ALPHA_MAX);
    }

    void scrollTo(float position) {
        int count = mList.getCount();
        mScrollCompleted = false;
        float fThreshold = 1.0f / count / 8;
        final Object[] sections = mSections;
        int sectionIndex;
        if (sections != null && sections.length > 1) {
            final int nSections = sections.length;
            int section = (int) (position * nSections);
            if (section >= nSections) {
                section = nSections - 1;
            }
            int exactSection = section;
            sectionIndex = section;
            int index = mSectionIndexer.getPositionForSection(section);
            int nextIndex = count;
            int prevIndex = index;
            int prevSection = section;
            int nextSection = section + 1;
            if (section < nSections - 1) {
                nextIndex = mSectionIndexer.getPositionForSection(section + 1);
            }
            if (nextIndex == index) {
                while (section > 0) {
                    section--;
                    prevIndex = mSectionIndexer.getPositionForSection(section);
                    if (prevIndex != index) {
                        prevSection = section;
                        sectionIndex = section;
                        break;
                    } else if (section == 0) {
                        sectionIndex = 0;
                        break;
                    }
                }
            }
            int nextNextSection = nextSection + 1;
            while (nextNextSection < nSections &&
                    mSectionIndexer.getPositionForSection(nextNextSection) == nextIndex) {
                nextNextSection++;
                nextSection++;
            }
            float fPrev = (float) prevSection / nSections;
            float fNext = (float) nextSection / nSections;
            if (prevSection == exactSection && position - fPrev < fThreshold) {
                index = prevIndex;
            } else {
                index = prevIndex + (int) ((nextIndex - prevIndex) * (position - fPrev)
                        / (fNext - fPrev));
            }
            if (index > count - 1) {
                index = count - 1;
            }
            if (mList instanceof ExpandableListView) {
                ExpandableListView expList = (ExpandableListView) mList;
                expList.setSelectionFromTop(expList.getFlatListPosition(
                        ExpandableListView.getPackedPositionForGroup(index + mListOffset)), 0);
            } else if (mList instanceof ListView) {
                mList.setSelectionFromTop(index + mListOffset, 0);
            } else {
                mList.setSelection(index + mListOffset);
            }
        } else {
            int index = (int) (position * count);
            if (index > count - 1) {
                index = count - 1;
            }
            if (mList instanceof ExpandableListView) {
                ExpandableListView expList = (ExpandableListView) mList;
                expList.setSelectionFromTop(expList.getFlatListPosition(
                        ExpandableListView.getPackedPositionForGroup(index + mListOffset)), 0);
            } else if (mList instanceof ListView) {
                mList.setSelectionFromTop(index + mListOffset, 0);
            } else {
                mList.setSelection(index + mListOffset);
            }
            sectionIndex = -1;
        }
        if (sectionIndex >= 0) {
            String text = mSectionText = sections[sectionIndex].toString();
            mDrawOverlay = (text.length() != 1 || text.charAt(0) != ' ') &&
                    sectionIndex < sections.length;
        } else {
            mDrawOverlay = false;
        }
    }

    public void setAlwaysShow(boolean alwaysShow) {
        mAlwaysShow = alwaysShow;
        if (alwaysShow) {
            mHandler.removeCallbacks(mScrollFade);
            setState(STATE_VISIBLE);
        } else if (mState == STATE_VISIBLE) {
            mHandler.postDelayed(mScrollFade, FADE_TIMEOUT);
        }
    }

    public void setScrollbarPosition(int position) {
        mPosition = position;
        switch (position) {
            default:
            case View.SCROLLBAR_POSITION_DEFAULT:
            case View.SCROLLBAR_POSITION_RIGHT:
                mOverlayDrawable = mOverlayDrawableRight;
                break;
            case View.SCROLLBAR_POSITION_LEFT:
                mOverlayDrawable = mOverlayDrawableLeft;
                break;
        }
    }

    public void setState(int state) {
        switch (state) {
            case STATE_NONE:
                mHandler.removeCallbacks(mScrollFade);
                mList.invalidate();
                break;
            case STATE_VISIBLE:
                if (mState != STATE_VISIBLE) {
                    resetThumbPos();
                }
            case STATE_DRAGGING:
                mHandler.removeCallbacks(mScrollFade);
                break;
            case STATE_EXIT:
                int viewWidth = mList.getWidth();
                mList.invalidate(viewWidth - mThumbW, mThumbY, viewWidth, mThumbY + mThumbH);
                break;
        }
        mState = state;
        refreshDrawableState();
    }

    void startPendingDrag() {
        mPendingDrag = true;
        mList.postDelayed(mDeferStartDrag, PENDING_DRAG_DELAY);
    }

    void stop() {
        setState(STATE_NONE);
    }

    private void useThumbDrawable(Context context, Drawable drawable) {
        mThumbDrawable = drawable;
        if (drawable instanceof NinePatchDrawable) {
            mThumbW = context.getResources().getDimensionPixelSize(
                    R.dimen.fastscroll_thumb_width);
            mThumbH = context.getResources().getDimensionPixelSize(
                    R.dimen.fastscroll_thumb_height);
        } else {
            mThumbW = drawable.getIntrinsicWidth();
            mThumbH = drawable.getIntrinsicHeight();
        }
        mChangedBounds = true;
    }
}
