
package org.holoeverywhere.widget;

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Locale;

import org.holoeverywhere.FontLoader;
import org.holoeverywhere.LayoutInflater;
import org.holoeverywhere.R;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.Configuration;
import android.content.res.TypedArray;
import android.database.DataSetObserver;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Paint.Align;
import android.graphics.Paint.Style;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.os.Build.VERSION;
import android.text.TextUtils;
import android.text.format.DateUtils;
import android.util.AttributeSet;
import android.util.DisplayMetrics;
import android.util.Log;
import android.util.TypedValue;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AbsListView;
import android.widget.AbsListView.OnScrollListener;
import android.widget.BaseAdapter;

public class CalendarView extends FrameLayout {
    public interface OnDateChangeListener {
        public void onSelectedDayChange(CalendarView view, int year, int month,
                int dayOfMonth);
    }

    private class ScrollStateRunnable implements Runnable {
        private int mNewState;
        private AbsListView mView;

        public void doScrollStateChange(AbsListView view, int scrollState) {
            mView = view;
            mNewState = scrollState;
            removeCallbacks(this);
            postDelayed(this, CalendarView.SCROLL_CHANGE_DELAY);
        }

        @Override
        @SuppressLint("NewApi")
        public void run() {
            mCurrentScrollState = mNewState;
            if (mNewState == OnScrollListener.SCROLL_STATE_IDLE
                    && mPreviousScrollState != OnScrollListener.SCROLL_STATE_IDLE) {
                View child = mView.getChildAt(0);
                if (child == null) {
                    return;
                }
                int dist = child.getBottom() - mListScrollTopOffset;
                if (dist > mListScrollTopOffset) {
                    int y = dist - (mIsScrollingUp ? child.getHeight() : 0);
                    if (VERSION.SDK_INT >= 11) {
                        mView.smoothScrollBy(y,
                                CalendarView.ADJUSTMENT_SCROLL_DURATION);
                    } else {
                        mView.scrollBy(0, y);
                    }
                }
            }
            mPreviousScrollState = mNewState;
        }
    }

    private class WeeksAdapter extends BaseAdapter implements OnTouchListener {
        class CalendarGestureListener extends
                GestureDetector.SimpleOnGestureListener {
            @Override
            public boolean onSingleTapUp(MotionEvent e) {
                return true;
            }
        }

        private Context context;
        private int mFocusedMonth;
        private GestureDetector mGestureDetector;
        private final Calendar mSelectedDate = Calendar.getInstance();
        private int mSelectedWeek;
        private int mTotalWeekCount;

        public WeeksAdapter(Context context) {
            this.context = context;
            mGestureDetector = new GestureDetector(context,
                    new CalendarGestureListener());
            init();
        }

        @Override
        public int getCount() {
            return mTotalWeekCount;
        }

        @Override
        public Object getItem(int position) {
            return null;
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        public Calendar getSelectedDay() {
            return mSelectedDate;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            WeekView weekView = null;
            if (convertView != null) {
                weekView = (WeekView) convertView;
            } else {
                weekView = new WeekView(context);
                android.widget.AbsListView.LayoutParams params = new android.widget.AbsListView.LayoutParams(
                        android.view.ViewGroup.LayoutParams.WRAP_CONTENT,
                        android.view.ViewGroup.LayoutParams.WRAP_CONTENT);
                weekView.setLayoutParams(params);
                weekView.setClickable(true);
                weekView.setOnTouchListener(this);
            }

            int selectedWeekDay = mSelectedWeek == position ? mSelectedDate
                    .get(Calendar.DAY_OF_WEEK) : -1;
            weekView.init(position, selectedWeekDay, mFocusedMonth);

            return weekView;
        }

        private void init() {
            mSelectedWeek = getWeeksSinceMinDate(mSelectedDate);
            mTotalWeekCount = getWeeksSinceMinDate(mMaxDate);
            if (mMinDate.get(Calendar.DAY_OF_WEEK) != mFirstDayOfWeek
                    || mMaxDate.get(Calendar.DAY_OF_WEEK) != mFirstDayOfWeek) {
                mTotalWeekCount++;
            }
        }

        private void onDateTapped(Calendar day) {
            setSelectedDay(day);
            setMonthDisplayed(day);
        }

        @Override
        public boolean onTouch(View v, MotionEvent event) {
            if (mListView.isEnabled() && mGestureDetector.onTouchEvent(event)) {
                WeekView weekView = (WeekView) v;
                if (!weekView.getDayFromLocation(event.getX(), mTempDate)) {
                    return true;
                }
                if (mTempDate.before(mMinDate) || mTempDate.after(mMaxDate)) {
                    return true;
                }
                onDateTapped(mTempDate);
                return true;
            }
            return false;
        }

        public void setFocusMonth(int month) {
            if (mFocusedMonth == month) {
                return;
            }
            mFocusedMonth = month;
            notifyDataSetChanged();
        }

        public void setSelectedDay(Calendar selectedDay) {
            if (selectedDay.get(Calendar.DAY_OF_YEAR) == mSelectedDate
                    .get(Calendar.DAY_OF_YEAR)
                    && selectedDay.get(Calendar.YEAR) == mSelectedDate
                            .get(Calendar.YEAR)) {
                return;
            }
            mSelectedDate.setTimeInMillis(selectedDay.getTimeInMillis());
            mSelectedWeek = getWeeksSinceMinDate(mSelectedDate);
            mFocusedMonth = mSelectedDate.get(Calendar.MONTH);
            notifyDataSetChanged();
        }
    }

    private class WeekView extends View {
        private String[] mDayNumbers;
        private final Paint mDrawPaint = new Paint();
        private Calendar mFirstDay;
        private boolean[] mFocusDay;
        private boolean mHasSelectedDay = false;
        private int mHeight;
        private int mLastWeekDayMonth = -1;
        private final Paint mMonthNumDrawPaint = new Paint();
        private int mMonthOfFirstWeekDay = -1;
        private int mNumCells;
        private int mSelectedDay = -1;
        private int mSelectedLeft = -1;
        private int mSelectedRight = -1;
        private final Rect mTempRect = new Rect();
        private int mWeek = -1;
        private int mWidth;

        public WeekView(Context context) {
            super(context);

            mHeight = (mListView.getHeight() - mListView.getPaddingTop() - mListView
                    .getPaddingBottom()) / mShownWeekCount;
            setPaintProperties();
        }

        private void drawBackground(Canvas canvas) {
            if (!mHasSelectedDay) {
                return;
            }
            mDrawPaint.setColor(mSelectedWeekBackgroundColor);
            mTempRect.top = mWeekSeperatorLineWidth;
            mTempRect.bottom = mHeight;
            mTempRect.left = mShowWeekNumber ? mWidth / mNumCells : 0;
            mTempRect.right = mSelectedLeft - 2;
            canvas.drawRect(mTempRect, mDrawPaint);
            mTempRect.left = mSelectedRight + 3;
            mTempRect.right = mWidth;
            canvas.drawRect(mTempRect, mDrawPaint);
        }

        private void drawSelectedDateVerticalBars(Canvas canvas) {
            if (!mHasSelectedDay) {
                return;
            }
            mSelectedDateVerticalBar.setBounds(mSelectedLeft
                    - mSelectedDateVerticalBarWidth / 2,
                    mWeekSeperatorLineWidth, mSelectedLeft
                            + mSelectedDateVerticalBarWidth / 2, mHeight);
            mSelectedDateVerticalBar.draw(canvas);
            mSelectedDateVerticalBar.setBounds(mSelectedRight
                    - mSelectedDateVerticalBarWidth / 2,
                    mWeekSeperatorLineWidth, mSelectedRight
                            + mSelectedDateVerticalBarWidth / 2, mHeight);
            mSelectedDateVerticalBar.draw(canvas);
        }

        private void drawWeekNumbers(Canvas canvas) {
            float textHeight = mDrawPaint.getTextSize();
            int y = (int) ((mHeight + textHeight) / 2)
                    - mWeekSeperatorLineWidth;
            int nDays = mNumCells;

            mDrawPaint.setTextAlign(Align.CENTER);
            int i = 0;
            int divisor = 2 * nDays;
            if (mShowWeekNumber) {
                mDrawPaint.setColor(mWeekNumberColor);
                int x = mWidth / divisor;
                canvas.drawText(mDayNumbers[0], x, y, mDrawPaint);
                i++;
            }
            for (; i < nDays; i++) {
                mMonthNumDrawPaint
                        .setColor(mFocusDay[i] ? mFocusedMonthDateColor
                                : mUnfocusedMonthDateColor);
                int x = (2 * i + 1) * mWidth / divisor;
                canvas.drawText(mDayNumbers[i], x, y, mMonthNumDrawPaint);
            }
        }

        private void drawWeekSeparators(Canvas canvas) {
            int firstFullyVisiblePosition = mListView.getFirstVisiblePosition();
            if (mListView.getChildAt(0).getTop() < 0) {
                firstFullyVisiblePosition++;
            }
            if (firstFullyVisiblePosition == mWeek) {
                return;
            }
            mDrawPaint.setColor(mWeekSeparatorLineColor);
            mDrawPaint.setStrokeWidth(mWeekSeperatorLineWidth);
            float x = mShowWeekNumber ? mWidth / mNumCells : 0;
            canvas.drawLine(x, 0, mWidth, 0, mDrawPaint);
        }

        public boolean getDayFromLocation(float x, Calendar outCalendar) {
            int dayStart = mShowWeekNumber ? mWidth / mNumCells : 0;
            if (x < dayStart || x > mWidth) {
                outCalendar.clear();
                return false;
            }
            int dayPosition = (int) ((x - dayStart) * mDaysPerWeek / (mWidth - dayStart));
            outCalendar.setTimeInMillis(mFirstDay.getTimeInMillis());
            outCalendar.add(Calendar.DAY_OF_MONTH, dayPosition);
            return true;
        }

        public Calendar getFirstDay() {
            return mFirstDay;
        }

        public int getMonthOfFirstWeekDay() {
            return mMonthOfFirstWeekDay;
        }

        public int getMonthOfLastWeekDay() {
            return mLastWeekDayMonth;
        }

        public void init(int weekNumber, int selectedWeekDay, int focusedMonth) {
            mSelectedDay = selectedWeekDay;
            mHasSelectedDay = mSelectedDay != -1;
            mNumCells = mShowWeekNumber ? mDaysPerWeek + 1 : mDaysPerWeek;
            mWeek = weekNumber;
            mTempDate.setTimeInMillis(mMinDate.getTimeInMillis());
            mTempDate.add(Calendar.WEEK_OF_YEAR, mWeek);
            mTempDate.setFirstDayOfWeek(mFirstDayOfWeek);
            mDayNumbers = new String[mNumCells];
            mFocusDay = new boolean[mNumCells];
            int i = 0;
            if (mShowWeekNumber) {
                mDayNumbers[0] = Integer.toString(mTempDate
                        .get(Calendar.WEEK_OF_YEAR));
                i++;
            }
            int diff = mFirstDayOfWeek - mTempDate.get(Calendar.DAY_OF_WEEK);
            mTempDate.add(Calendar.DAY_OF_MONTH, diff);
            mFirstDay = (Calendar) mTempDate.clone();
            mMonthOfFirstWeekDay = mTempDate.get(Calendar.MONTH);
            for (; i < mNumCells; i++) {
                mFocusDay[i] = mTempDate.get(Calendar.MONTH) == focusedMonth;
                if (mTempDate.before(mMinDate) || mTempDate.after(mMaxDate)) {
                    mDayNumbers[i] = "";
                } else {
                    mDayNumbers[i] = Integer.toString(mTempDate
                            .get(Calendar.DAY_OF_MONTH));
                }
                mTempDate.add(Calendar.DAY_OF_MONTH, 1);
            }
            if (mTempDate.get(Calendar.DAY_OF_MONTH) == 1) {
                mTempDate.add(Calendar.DAY_OF_MONTH, -1);
            }
            mLastWeekDayMonth = mTempDate.get(Calendar.MONTH);
            updateSelectionPositions();
        }

        @Override
        protected void onDraw(Canvas canvas) {
            drawBackground(canvas);
            drawWeekNumbers(canvas);
            drawWeekSeparators(canvas);
            drawSelectedDateVerticalBars(canvas);
        }

        @Override
        protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
            setMeasuredDimension(MeasureSpec.getSize(widthMeasureSpec), mHeight);
        }

        @Override
        protected void onSizeChanged(int w, int h, int oldw, int oldh) {
            mWidth = w;
            updateSelectionPositions();
        }

        private void setPaintProperties() {
            mDrawPaint.setFakeBoldText(false);
            mDrawPaint.setAntiAlias(true);
            mDrawPaint.setTextSize(mDateTextSize);
            mDrawPaint.setStyle(Style.FILL);

            mMonthNumDrawPaint.setFakeBoldText(true);
            mMonthNumDrawPaint.setAntiAlias(true);
            mMonthNumDrawPaint.setTextSize(mDateTextSize);
            mMonthNumDrawPaint.setColor(mFocusedMonthDateColor);
            mMonthNumDrawPaint.setStyle(Style.FILL);
            mMonthNumDrawPaint.setTextAlign(Align.CENTER);
        }

        private void updateSelectionPositions() {
            if (mHasSelectedDay) {
                int selectedPosition = mSelectedDay - mFirstDayOfWeek;
                if (selectedPosition < 0) {
                    selectedPosition += 7;
                }
                if (mShowWeekNumber) {
                    selectedPosition++;
                }
                mSelectedLeft = selectedPosition * mWidth / mNumCells;
                mSelectedRight = (selectedPosition + 1) * mWidth / mNumCells;
            }
        }
    }

    private static final int ADJUSTMENT_SCROLL_DURATION = 500;
    private static final String DATE_FORMAT = "MM/dd/yyyy";
    private static final int DAYS_PER_WEEK = 7;
    private static final String DEFAULT_MAX_DATE = "01/01/2100";
    private static final String DEFAULT_MIN_DATE = "01/01/1900";
    private static final boolean DEFAULT_SHOW_WEEK_NUMBER = true;
    private static final int DEFAULT_SHOWN_WEEK_COUNT = 6;
    private static final int DEFAULT_WEEK_DAY_TEXT_APPEARANCE_RES_ID = -1;
    private static final int GOTO_SCROLL_DURATION = 1000;
    private static final String LOG_TAG = CalendarView.class.getSimpleName();
    private static final long MILLIS_IN_DAY = 86400000L;
    private static final long MILLIS_IN_WEEK = CalendarView.DAYS_PER_WEEK
            * CalendarView.MILLIS_IN_DAY;
    private static final int SCROLL_CHANGE_DELAY = 40;
    private static final int SCROLL_HYST_WEEKS = 2;
    private static final int UNSCALED_BOTTOM_BUFFER = 20;
    private static final int UNSCALED_LIST_SCROLL_TOP_OFFSET = 2;
    private static final int UNSCALED_SELECTED_DATE_VERTICAL_BAR_WIDTH = 6;
    private static final int UNSCALED_WEEK_MIN_VISIBLE_HEIGHT = 12;
    private static final int UNSCALED_WEEK_SEPARATOR_LINE_WIDTH = 1;
    private WeeksAdapter mAdapter;
    private int mBottomBuffer = 20;
    private Locale mCurrentLocale;
    private int mCurrentMonthDisplayed;
    private int mCurrentScrollState = OnScrollListener.SCROLL_STATE_IDLE;
    private int mCurrentYearDisplayed;
    private final java.text.DateFormat mDateFormat = new SimpleDateFormat(
            CalendarView.DATE_FORMAT);
    private final int mDateTextSize;
    private String[] mDayLabels;
    private ViewGroup mDayNamesHeader;
    private int mDaysPerWeek = 7;
    private Calendar mFirstDayOfMonth;
    private int mFirstDayOfWeek;
    private final int mFocusedMonthDateColor;
    private float mFriction = .05f;
    private boolean mIsScrollingUp = false;
    private int mListScrollTopOffset = 2;
    private ListView mListView;
    private Calendar mMaxDate;
    private Calendar mMinDate;
    private TextView mMonthName;
    private OnDateChangeListener mOnDateChangeListener;
    private long mPreviousScrollPosition;
    private int mPreviousScrollState = OnScrollListener.SCROLL_STATE_IDLE;
    private ScrollStateRunnable mScrollStateChangedRunnable = new ScrollStateRunnable();
    private final Drawable mSelectedDateVerticalBar;
    private final int mSelectedDateVerticalBarWidth;
    private final int mSelectedWeekBackgroundColor;
    private int mShownWeekCount;
    private boolean mShowWeekNumber;
    private Calendar mTempDate;
    private final int mUnfocusedMonthDateColor;
    private float mVelocityScale = 0.333f;
    private int mWeekMinVisibleHeight = 12;
    private final int mWeekNumberColor;
    private final int mWeekSeparatorLineColor;
    private final int mWeekSeperatorLineWidth;

    public CalendarView(Context context) {
        this(context, null);
    }

    public CalendarView(Context context, AttributeSet attrs) {
        this(context, attrs, R.attr.calendarViewStyle);
    }

    public CalendarView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        setCurrentLocale(Locale.getDefault());
        TypedArray attributesArray = context.obtainStyledAttributes(attrs,
                R.styleable.CalendarView, defStyle, R.style.Holo_CalendarView);
        mShowWeekNumber = attributesArray.getBoolean(
                R.styleable.CalendarView_showWeekNumber,
                CalendarView.DEFAULT_SHOW_WEEK_NUMBER);
        mFirstDayOfWeek = attributesArray.getInt(
                R.styleable.CalendarView_firstDayOfWeek, 1);
        String minDate = attributesArray
                .getString(R.styleable.CalendarView_minDate);
        if (TextUtils.isEmpty(minDate) || !parseDate(minDate, mMinDate)) {
            parseDate(CalendarView.DEFAULT_MIN_DATE, mMinDate);
        }
        String maxDate = attributesArray
                .getString(R.styleable.CalendarView_maxDate);
        if (TextUtils.isEmpty(maxDate) || !parseDate(maxDate, mMaxDate)) {
            parseDate(CalendarView.DEFAULT_MAX_DATE, mMaxDate);
        }
        if (mMaxDate.before(mMinDate)) {
            throw new IllegalArgumentException(
                    "Max date cannot be before min date.");
        }
        mShownWeekCount = attributesArray.getInt(
                R.styleable.CalendarView_shownWeekCount,
                CalendarView.DEFAULT_SHOWN_WEEK_COUNT);
        mSelectedWeekBackgroundColor = attributesArray.getColor(
                R.styleable.CalendarView_selectedWeekBackgroundColor, 0);
        mFocusedMonthDateColor = attributesArray.getColor(
                R.styleable.CalendarView_focusedMonthDateColor, 0);
        mUnfocusedMonthDateColor = attributesArray.getColor(
                R.styleable.CalendarView_unfocusedMonthDateColor, 0);
        mWeekSeparatorLineColor = attributesArray.getColor(
                R.styleable.CalendarView_weekSeparatorLineColor, 0);
        mWeekNumberColor = attributesArray.getColor(
                R.styleable.CalendarView_weekNumberColor, 0);
        mSelectedDateVerticalBar = attributesArray
                .getDrawable(R.styleable.CalendarView_selectedDateVerticalBar);
        attributesArray.getResourceId(
                R.styleable.CalendarView_dateTextAppearance,
                android.R.style.TextAppearance_Small);
        mDateTextSize = (int) (12 * getContext().getResources()
                .getDisplayMetrics().density);
        int weekDayTextAppearanceResId = attributesArray.getResourceId(
                R.styleable.CalendarView_weekDayTextAppearance,
                CalendarView.DEFAULT_WEEK_DAY_TEXT_APPEARANCE_RES_ID);
        attributesArray.recycle();
        DisplayMetrics displayMetrics = getResources().getDisplayMetrics();
        mWeekMinVisibleHeight = (int) TypedValue.applyDimension(
                TypedValue.COMPLEX_UNIT_DIP,
                CalendarView.UNSCALED_WEEK_MIN_VISIBLE_HEIGHT, displayMetrics);
        mListScrollTopOffset = (int) TypedValue.applyDimension(
                TypedValue.COMPLEX_UNIT_DIP,
                CalendarView.UNSCALED_LIST_SCROLL_TOP_OFFSET, displayMetrics);
        mBottomBuffer = (int) TypedValue.applyDimension(
                TypedValue.COMPLEX_UNIT_DIP,
                CalendarView.UNSCALED_BOTTOM_BUFFER, displayMetrics);
        mSelectedDateVerticalBarWidth = (int) TypedValue.applyDimension(
                TypedValue.COMPLEX_UNIT_DIP,
                CalendarView.UNSCALED_SELECTED_DATE_VERTICAL_BAR_WIDTH,
                displayMetrics);
        mWeekSeperatorLineWidth = (int) TypedValue
                .applyDimension(TypedValue.COMPLEX_UNIT_DIP,
                        CalendarView.UNSCALED_WEEK_SEPARATOR_LINE_WIDTH,
                        displayMetrics);
        LayoutInflater.inflate(context, R.layout.calendar_view, this, true);
        FontLoader.apply(this);
        mListView = (ListView) findViewById(R.id.list);
        mDayNamesHeader = (ViewGroup) findViewById(R.id.day_names);
        mMonthName = (TextView) findViewById(R.id.month_name);
        setUpHeader(weekDayTextAppearanceResId);
        setUpListView();
        setUpAdapter();
        mTempDate.setTimeInMillis(System.currentTimeMillis());
        if (mTempDate.before(mMinDate)) {
            goTo(mMinDate, false, true, true);
        } else if (mMaxDate.before(mTempDate)) {
            goTo(mMaxDate, false, true, true);
        } else {
            goTo(mTempDate, false, true, true);
        }
        invalidate();
    }

    private Calendar getCalendarForLocale(Calendar oldCalendar, Locale locale) {
        if (oldCalendar == null) {
            return Calendar.getInstance(locale);
        } else {
            final long currentTimeMillis = oldCalendar.getTimeInMillis();
            Calendar newCalendar = Calendar.getInstance(locale);
            newCalendar.setTimeInMillis(currentTimeMillis);
            return newCalendar;
        }
    }

    public long getDate() {
        return mAdapter.mSelectedDate.getTimeInMillis();
    }

    public int getFirstDayOfWeek() {
        return mFirstDayOfWeek;
    }

    public long getMaxDate() {
        return mMaxDate.getTimeInMillis();
    }

    public long getMinDate() {
        return mMinDate.getTimeInMillis();
    }

    public boolean getShowWeekNumber() {
        return mShowWeekNumber;
    }

    private int getWeeksSinceMinDate(Calendar date) {
        if (date.before(mMinDate)) {
            throw new IllegalArgumentException("fromDate: "
                    + mMinDate.getTime() + " does not precede toDate: "
                    + date.getTime());
        }
        long endTimeMillis = date.getTimeInMillis()
                + date.getTimeZone().getOffset(date.getTimeInMillis());
        long startTimeMillis = mMinDate.getTimeInMillis()
                + mMinDate.getTimeZone().getOffset(mMinDate.getTimeInMillis());
        long dayOffsetMillis = (mMinDate.get(Calendar.DAY_OF_WEEK) - mFirstDayOfWeek)
                * CalendarView.MILLIS_IN_DAY;
        return (int) ((endTimeMillis - startTimeMillis + dayOffsetMillis) / CalendarView.MILLIS_IN_WEEK);
    }

    @SuppressLint("NewApi")
    private void goTo(Calendar date, boolean animate, boolean setSelected,
            boolean forceScroll) {
        if (date.before(mMinDate) || date.after(mMaxDate)) {
            throw new IllegalArgumentException("Time not between "
                    + mMinDate.getTime() + " and " + mMaxDate.getTime());
        }
        int firstFullyVisiblePosition = mListView.getFirstVisiblePosition();
        View firstChild = mListView.getChildAt(0);
        if (firstChild != null && firstChild.getTop() < 0) {
            firstFullyVisiblePosition++;
        }
        int lastFullyVisiblePosition = firstFullyVisiblePosition
                + mShownWeekCount - 1;
        if (firstChild != null && firstChild.getTop() > mBottomBuffer) {
            lastFullyVisiblePosition--;
        }
        if (setSelected) {
            mAdapter.setSelectedDay(date);
        }
        int position = getWeeksSinceMinDate(date);
        if (position < firstFullyVisiblePosition
                || position > lastFullyVisiblePosition || forceScroll) {
            mFirstDayOfMonth.setTimeInMillis(date.getTimeInMillis());
            mFirstDayOfMonth.set(Calendar.DAY_OF_MONTH, 1);
            setMonthDisplayed(mFirstDayOfMonth);
            if (mFirstDayOfMonth.before(mMinDate)) {
                position = 0;
            } else {
                position = getWeeksSinceMinDate(mFirstDayOfMonth);
            }
            mPreviousScrollState = OnScrollListener.SCROLL_STATE_FLING;
            if (animate && VERSION.SDK_INT >= 11) {
                mListView
                        .smoothScrollToPositionFromTop(position,
                                mListScrollTopOffset,
                                CalendarView.GOTO_SCROLL_DURATION);
            } else {
                mListView.setSelectionFromTop(position, mListScrollTopOffset);
                onScrollStateChanged(mListView,
                        OnScrollListener.SCROLL_STATE_IDLE);
            }
        } else if (setSelected) {
            setMonthDisplayed(date);
        }
    }

    @Override
    public boolean isEnabled() {
        return mListView.isEnabled();
    }

    private boolean isSameDate(Calendar firstDate, Calendar secondDate) {
        return firstDate.get(Calendar.DAY_OF_YEAR) == secondDate
                .get(Calendar.DAY_OF_YEAR)
                && firstDate.get(Calendar.YEAR) == secondDate
                        .get(Calendar.YEAR);
    }

    @SuppressLint("NewApi")
    @Override
    protected void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        setCurrentLocale(newConfig.locale);
    }

    private void onScroll(AbsListView view, int firstVisibleItem,
            int visibleItemCount, int totalItemCount) {
        WeekView child = (WeekView) view.getChildAt(0);
        if (child == null) {
            return;
        }
        long currScroll = view.getFirstVisiblePosition() * child.getHeight()
                - child.getBottom();
        if (currScroll < mPreviousScrollPosition) {
            mIsScrollingUp = true;
        } else if (currScroll > mPreviousScrollPosition) {
            mIsScrollingUp = false;
        } else {
            return;
        }
        int offset = child.getBottom() < mWeekMinVisibleHeight ? 1 : 0;
        if (mIsScrollingUp) {
            child = (WeekView) view.getChildAt(CalendarView.SCROLL_HYST_WEEKS
                    + offset);
        } else if (offset != 0) {
            child = (WeekView) view.getChildAt(offset);
        }
        int month;
        if (mIsScrollingUp) {
            month = child.getMonthOfFirstWeekDay();
        } else {
            month = child.getMonthOfLastWeekDay();
        }
        int monthDiff;
        if (mCurrentMonthDisplayed == 11 && month == 0) {
            monthDiff = 1;
        } else if (mCurrentMonthDisplayed == 0 && month == 11) {
            monthDiff = -1;
        } else {
            monthDiff = month - mCurrentMonthDisplayed;
        }
        if (!mIsScrollingUp && monthDiff > 0 || mIsScrollingUp && monthDiff < 0) {
            Calendar firstDay = child.getFirstDay();
            if (mIsScrollingUp) {
                firstDay.add(Calendar.DAY_OF_MONTH, -CalendarView.DAYS_PER_WEEK);
            } else {
                firstDay.add(Calendar.DAY_OF_MONTH, CalendarView.DAYS_PER_WEEK);
            }
            setMonthDisplayed(firstDay);
        }
        mPreviousScrollPosition = currScroll;
        mPreviousScrollState = mCurrentScrollState;
    }

    private void onScrollStateChanged(AbsListView view, int scrollState) {
        mScrollStateChangedRunnable.doScrollStateChange(view, scrollState);
    }

    private boolean parseDate(String date, Calendar outDate) {
        try {
            outDate.setTime(mDateFormat.parse(date));
            return true;
        } catch (ParseException e) {
            Log.w(CalendarView.LOG_TAG, "Date: " + date + " not in format: "
                    + CalendarView.DATE_FORMAT);
            return false;
        }
    }

    private void setCurrentLocale(Locale locale) {
        if (locale.equals(mCurrentLocale)) {
            return;
        }

        mCurrentLocale = locale;
        mTempDate = getCalendarForLocale(mTempDate, locale);
        mFirstDayOfMonth = getCalendarForLocale(mFirstDayOfMonth, locale);
        mMinDate = getCalendarForLocale(mMinDate, locale);
        mMaxDate = getCalendarForLocale(mMaxDate, locale);
    }

    public void setDate(long date) {
        setDate(date, false, false);
    }

    public void setDate(long date, boolean animate, boolean center) {
        mTempDate.setTimeInMillis(date);
        if (isSameDate(mTempDate, mAdapter.mSelectedDate)) {
            return;
        }
        goTo(mTempDate, animate, true, center);
    }

    @Override
    public void setEnabled(boolean enabled) {
        mListView.setEnabled(enabled);
    }

    public void setFirstDayOfWeek(int firstDayOfWeek) {
        if (mFirstDayOfWeek == firstDayOfWeek) {
            return;
        }
        mFirstDayOfWeek = firstDayOfWeek;
        mAdapter.init();
        mAdapter.notifyDataSetChanged();
        setUpHeader(CalendarView.DEFAULT_WEEK_DAY_TEXT_APPEARANCE_RES_ID);
    }

    public void setMaxDate(long maxDate) {
        mTempDate.setTimeInMillis(maxDate);
        if (isSameDate(mTempDate, mMaxDate)) {
            return;
        }
        mMaxDate.setTimeInMillis(maxDate);
        mAdapter.init();
        Calendar date = mAdapter.mSelectedDate;
        if (date.after(mMaxDate)) {
            setDate(mMaxDate.getTimeInMillis());
        } else {
            goTo(date, false, true, false);
        }
    }

    public void setMinDate(long minDate) {
        mTempDate.setTimeInMillis(minDate);
        if (isSameDate(mTempDate, mMinDate)) {
            return;
        }
        mMinDate.setTimeInMillis(minDate);
        Calendar date = mAdapter.mSelectedDate;
        if (date.before(mMinDate)) {
            mAdapter.setSelectedDay(mMinDate);
        }
        mAdapter.init();
        if (date.before(mMinDate)) {
            setDate(mTempDate.getTimeInMillis());
        } else {
            goTo(date, false, true, false);
        }
    }

    private void setMonthDisplayed(Calendar calendar) {
        final int newMonthDisplayed = calendar.get(Calendar.MONTH);
        final int newYearDisplayed = calendar.get(Calendar.YEAR);
        if (mCurrentMonthDisplayed != newMonthDisplayed
                || mCurrentYearDisplayed != newYearDisplayed) {
            mCurrentMonthDisplayed = newMonthDisplayed;
            mCurrentYearDisplayed = newYearDisplayed;
            mAdapter.setFocusMonth(mCurrentMonthDisplayed);
            final int flags = DateUtils.FORMAT_SHOW_DATE
                    | DateUtils.FORMAT_NO_MONTH_DAY
                    | DateUtils.FORMAT_SHOW_YEAR;
            final long millis = calendar.getTimeInMillis();
            String newMonthName = DateUtils.formatDateRange(getContext(),
                    millis, millis, flags);
            mMonthName.setText(newMonthName);
            mMonthName.invalidate();
        }
    }

    public void setOnDateChangeListener(OnDateChangeListener listener) {
        mOnDateChangeListener = listener;
    }

    public void setShowWeekNumber(boolean showWeekNumber) {
        if (mShowWeekNumber == showWeekNumber) {
            return;
        }
        mShowWeekNumber = showWeekNumber;
        mAdapter.notifyDataSetChanged();
        setUpHeader(CalendarView.DEFAULT_WEEK_DAY_TEXT_APPEARANCE_RES_ID);
    }

    private void setUpAdapter() {
        if (mAdapter == null) {
            mAdapter = new WeeksAdapter(getContext());
            mAdapter.registerDataSetObserver(new DataSetObserver() {
                @Override
                public void onChanged() {
                    if (mOnDateChangeListener != null) {
                        Calendar selectedDay = mAdapter.getSelectedDay();
                        mOnDateChangeListener.onSelectedDayChange(
                                CalendarView.this,
                                selectedDay.get(Calendar.YEAR),
                                selectedDay.get(Calendar.MONTH),
                                selectedDay.get(Calendar.DAY_OF_MONTH));
                    }
                }
            });
            mListView.setAdapter(mAdapter);
        }
        mAdapter.notifyDataSetChanged();
    }

    private void setUpHeader(int weekDayTextAppearanceResId) {
        mDayLabels = new String[mDaysPerWeek];
        for (int i = mFirstDayOfWeek, count = mFirstDayOfWeek + mDaysPerWeek; i < count; i++) {
            int calendarDay = i > Calendar.SATURDAY ? i - Calendar.SATURDAY : i;
            mDayLabels[i - mFirstDayOfWeek] = DateUtils.getDayOfWeekString(
                    calendarDay, DateUtils.LENGTH_SHORTEST);
        }
        TextView label = (TextView) mDayNamesHeader.getChildAt(0);
        if (mShowWeekNumber) {
            label.setVisibility(View.VISIBLE);
        } else {
            label.setVisibility(View.GONE);
        }
        for (int i = 1, count = mDayNamesHeader.getChildCount(); i < count; i++) {
            label = (TextView) mDayNamesHeader.getChildAt(i);
            if (weekDayTextAppearanceResId > -1) {
                label.setTextAppearance(getContext(),
                        weekDayTextAppearanceResId);
            }
            if (i < mDaysPerWeek + 1) {
                label.setText(mDayLabels[i - 1]);
                label.setVisibility(View.VISIBLE);
            } else {
                label.setVisibility(View.GONE);
            }
        }
        mDayNamesHeader.invalidate();
    }

    @SuppressLint("NewApi")
    private void setUpListView() {
        mListView.setDivider(null);
        mListView.setItemsCanFocus(true);
        mListView.setVerticalScrollBarEnabled(false);
        mListView.setOnScrollListener(new OnScrollListener() {
            @Override
            public void onScroll(AbsListView view, int firstVisibleItem,
                    int visibleItemCount, int totalItemCount) {
                CalendarView.this.onScroll(view, firstVisibleItem,
                        visibleItemCount, totalItemCount);
            }

            @Override
            public void onScrollStateChanged(AbsListView view, int scrollState) {
                CalendarView.this.onScrollStateChanged(view, scrollState);
            }
        });
        if (VERSION.SDK_INT >= 11) {
            mListView.setFriction(mFriction);
            mListView.setVelocityScale(mVelocityScale);
        }
    }
}
