package com.mapswithme.maps.bookmarks;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.ParcelablePoint;

import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Paint.Style;
import android.graphics.Path;
import android.graphics.Point;
import android.graphics.PorterDuff.Mode;
import android.graphics.Rect;
import android.text.TextPaint;
import android.text.TextUtils;
import android.text.TextUtils.TruncateAt;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;

public class PopupLayout extends View
{
  private static final String DEACTIVATION = "deactivation";
  private static final int VERTICAL_MARGIN = 10;
  private final int mThriangleHeight;
  private Bitmap mAddButton;
  private Bitmap mEditButton;
  private Bitmap mPin;
  private Bitmap mPopup;

  volatile private Bookmark mBmk;
  private Paint mBackgroundPaint;
  private Paint mBorderPaint;
  private Paint mButtonPaint = new Paint();
  private Path mPopupPath;
  private TextPaint mTextPaint;
  private Rect mTextBounds = new Rect();
  private Rect mPopupRect = new Rect();
  private Point mPopupAnchor = new Point();

  private int mWidth;
  private int mHeight;

  public PopupLayout(Context context)
  {
    this(context, null, 0);
  }

  public PopupLayout(Context context, AttributeSet attrs)
  {
    this(context, attrs, 0);
  }

  public PopupLayout(Context context, AttributeSet attrs, int defStyle)
  {
    super(context, attrs, defStyle);
    mAddButton = BitmapFactory.decodeResource(getContext().getResources(), R.drawable.add);
    mEditButton = BitmapFactory.decodeResource(getContext().getResources(), R.drawable.arrow);
    mPin = BitmapFactory.decodeResource(getContext().getResources(), R.drawable.placemark_red);

    mThriangleHeight = (int) (10 * getResources().getDisplayMetrics().density);
    mBackgroundPaint = new Paint();
    mBackgroundPaint.setColor(Color.BLACK);
    mBackgroundPaint.setStyle(Paint.Style.FILL);
    mBorderPaint = new Paint();
    mBorderPaint.setStyle(Style.STROKE);
    mBorderPaint.setColor(Color.GRAY);
    mBorderPaint.setStrokeWidth(2);
    mBorderPaint.setAntiAlias(true);
    mPopupPath = new Path();

    mTextPaint = new TextPaint();
    mTextPaint.setTextSize(20 * getResources().getDisplayMetrics().density);
    mTextPaint.setAntiAlias(true);
    mTextPaint.setColor(Color.WHITE);
  }

  public synchronized void activate(final Bookmark bmk)
  {
    mBmk = bmk;
    nDrawBookmark(bmk.getPosition().x, bmk.getPosition().y);
    mPopup = prepareBitmap(mBmk);
    postInvalidate();
  }

  public synchronized void deactivate()
  {
    mBmk = null;
    nRemoveBookmark();
    if(mPopup != null)
    {
      Bitmap b = mPopup;
      mPopup = null;
      b.recycle();
    }
    postInvalidate();
  }

  public synchronized boolean isActive()
  {
    return mBmk != null;
  }

  private Point anchor;

  @Override
  protected void onSizeChanged(int w, int h, int oldw, int oldh)
  {
    super.onSizeChanged(w, h, oldw, oldh);
    anchor = new Point(w / 2, h / 2);
  }

  public void requestInvalidation()
  {
    if (isActive())
      postInvalidate();
  }

  private Bitmap prepareBitmap(Bookmark bmk)
  {
    anchor = bmk.getPosition();

    Bitmap btn;
    if (bmk.isPreviewBookmark())
    {
      btn = mAddButton;
    }
    else
    {
      btn = mEditButton;
      BookmarkManager.getBookmarkManager(getContext()).getCategoryById(bmk.getCategoryId()).setVisibility(true);
    }

    String bmkName = bmk.getName();
    int pWidth = Math.min(getWidth(), getHeight()) - VERTICAL_MARGIN;
    int pHeight = mHeight = btn.getHeight() + 10;
    int maxTextWidth = pWidth - btn.getWidth() - 10;
    int currentTextWidth = Math.round(mTextPaint.measureText(bmkName));
    String text;
    if (currentTextWidth > maxTextWidth)
    {
      text = TextUtils.ellipsize(bmkName, mTextPaint, maxTextWidth, TruncateAt.END).toString();
      currentTextWidth = Math.round(mTextPaint.measureText(text));;
    }
    else
    {
      text = bmkName;
    }
    pWidth = mWidth = currentTextWidth + btn.getWidth() + 10;


    mPopupPath.reset();
    mPopupPath.moveTo(0, 0);
    mPopupPath.lineTo(pWidth, 0);
    mPopupPath.lineTo(pWidth, 0 + pHeight);
    mPopupPath.lineTo(mWidth/2 + mThriangleHeight, pHeight);
    mPopupPath.lineTo(mWidth/2, pHeight + mThriangleHeight);
    mPopupPath.lineTo(mWidth/2 - mThriangleHeight, pHeight);
    mPopupPath.lineTo(0, pHeight);
    mPopupPath.lineTo(0, 0);

    Bitmap b = Bitmap.createBitmap(pWidth, pHeight + mThriangleHeight, Config.ARGB_8888);
    Canvas canvas = new Canvas(b);
    canvas.drawPath(mPopupPath, mBackgroundPaint);
    canvas.drawBitmap(btn, pWidth - btn.getWidth()-5, + 5, mButtonPaint);
    canvas.drawPath(mPopupPath, mBorderPaint);
    mTextPaint.getTextBounds(text, 0, text.length(), mTextBounds);

    int textHeight = mTextBounds.bottom - mTextBounds.top;
    canvas.drawText(text, 2,
                    - (pHeight - textHeight) / 2 + pHeight,
                    mTextPaint);

    return b;
  }

  @Override
  protected void onDraw(Canvas canvas)
  {
    Bookmark bmk = mBmk;
    if (bmk != null)
    {
      Log.d(DEACTIVATION, "i'm still drawing");
      int pinHeight = Math.round(35/1.5f * getResources().getDisplayMetrics().density);
      mPopupAnchor.x = bmk.getPosition().x - mWidth / 2;
      mPopupAnchor.y = bmk.getPosition().y - pinHeight - mThriangleHeight - mHeight;

      mPopupRect.left = mPopupAnchor.x;
      mPopupRect.top = mPopupAnchor.y;
      mPopupRect.right = mPopupAnchor.x + mWidth;
      mPopupRect.bottom = mPopupAnchor.y + mHeight;

      if (mPopup != null)
      {
        canvas.drawBitmap(mPopup, mPopupAnchor.x, mPopupAnchor.y, mButtonPaint);
      }
    }
    else
    {
      canvas.drawColor(Color.RED);
      super.onDraw(canvas);
    }

  }

  private native void nDrawBookmark(double x, double y);
  private native void nRemoveBookmark();

  /**
   *
   * @param x
   * @param y
   * @return true if we start {@link BookmarkActivity}, false otherwise
   */
  public synchronized boolean handleClick(int x, int y, boolean ispro)
  {
    if (mBmk != null)
    {
      if ( mPopupRect.contains(x, y) )
      {
        if (ispro)
        {
          if (mBmk.isPreviewBookmark())
          {
            getContext().startActivity(new Intent(getContext(), BookmarkActivity.class).
                          putExtra(BookmarkActivity.BOOKMARK_POSITION, new ParcelablePoint(mBmk.getPosition())).
                          putExtra(BookmarkActivity.BOOKMARK_NAME, mBmk.getName()));
          }
          else
          {
            getContext().startActivity(
                                       new Intent(getContext(), BookmarkActivity.class)
                                       .putExtra(
                                                 BookmarkActivity.PIN,
                                                 new ParcelablePoint(mBmk.getCategoryId(), mBmk.getBookmarkId()
                                               )
                                       ));
          }
        }
        return true;
      }
      deactivate();
    }
    return false;
  }


}
