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
import android.graphics.Rect;
import android.text.TextPaint;
import android.text.TextUtils;
import android.text.TextUtils.TruncateAt;
import android.util.AttributeSet;
import android.view.View;

public class PopupLayout extends View
{
  private final int m_thriangleHeight;
  private Bitmap m_AddButton;
  private Bitmap m_editButton;
  private Bitmap m_pin;
  private Bitmap m_popup;

  private Bookmark m_bmk;
  private Paint m_backgroundPaint;
  private Paint m_borderPaint;
  private Paint m_buttonPaint = new Paint();
  private Path m_popupPath;
  private TextPaint m_textPaint;
  private Rect m_textBounds = new Rect();
  private Rect m_popupRect = new Rect();
  private Point m_popupAnchor = new Point();

  private int m_width;
  private int m_height;

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
    m_AddButton = BitmapFactory.decodeResource(getContext().getResources(), R.drawable.add);
    m_editButton = BitmapFactory.decodeResource(getContext().getResources(), R.drawable.arrow);
    m_pin = BitmapFactory.decodeResource(getContext().getResources(), R.drawable.placemark_red);

    m_thriangleHeight = (int) (10 * getResources().getDisplayMetrics().density);
    m_backgroundPaint = new Paint();
    m_backgroundPaint.setColor(0x50677800);
    m_backgroundPaint.setStyle(Paint.Style.FILL);
    m_borderPaint = new Paint();
    m_borderPaint.setStyle(Style.STROKE);
    m_borderPaint.setColor(Color.GRAY);
    m_borderPaint.setStrokeWidth(2);
    m_borderPaint.setAntiAlias(true);
    m_popupPath = new Path();

    m_textPaint = new TextPaint();
    m_textPaint.setTextSize(20 * getResources().getDisplayMetrics().density);
    m_textPaint.setAntiAlias(true);
  }

  public synchronized void activate(final Bookmark bmk)
  {
    m_bmk = bmk;
    m_popup = prepareBitmap(m_bmk);
    postInvalidate();
  }

  public synchronized void deactivate()
  {
    m_bmk = null;
    nRemoveBookmark();
    if(m_popup != null)
    {
      Bitmap b = m_popup;
      m_popup = null;
      b.recycle();
    }
    postInvalidate();
  }

  public synchronized boolean isActive()
  {
    return m_bmk != null;
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
      btn = m_AddButton;
      nDrawBookmark(bmk.getPosition().x, bmk.getPosition().y);
    }
    else
    {
      btn = m_editButton;
      BookmarkManager.getBookmarkManager(getContext()).getCategoryById(bmk.getCategoryId()).setVisibility(true);
    }

    String bmkName = bmk.getName();
    int pWidth = getWidth() / 2;
    int pHeight = m_height = btn.getHeight() + 10;
    int maxTextWidth = pWidth - btn.getWidth() - 10;
    int currentTextWidth = Math.round(m_textPaint.measureText(bmkName));
    String text;
    if (currentTextWidth > maxTextWidth)
    {
      text = TextUtils.ellipsize(bmkName, m_textPaint, maxTextWidth, TruncateAt.END).toString();
      currentTextWidth = Math.round(m_textPaint.measureText(text));;
    }
    else
    {
      text = bmkName;
    }
    pWidth = m_width = currentTextWidth + btn.getWidth() + 10;


    m_popupPath.reset();
    m_popupPath.moveTo(0, 0);
    m_popupPath.lineTo(pWidth, 0);
    m_popupPath.lineTo(pWidth, 0 + pHeight);
    m_popupPath.lineTo(m_width/2 + m_thriangleHeight, pHeight);
    m_popupPath.lineTo(m_width/2, pHeight + m_thriangleHeight);
    m_popupPath.lineTo(m_width/2 - m_thriangleHeight, pHeight);
    m_popupPath.lineTo(0, pHeight);
    m_popupPath.lineTo(0, 0);

    Bitmap b = Bitmap.createBitmap(pWidth, pHeight + m_thriangleHeight, Config.ARGB_8888);
    Canvas canvas = new Canvas(b);
    canvas.drawPath(m_popupPath, m_backgroundPaint);
    canvas.drawBitmap(btn, pWidth - btn.getWidth()-5, + 5, m_buttonPaint);
    canvas.drawPath(m_popupPath, m_borderPaint);
    m_textPaint.getTextBounds(text, 0, text.length(), m_textBounds);

    int textHeight = m_textBounds.bottom - m_textBounds.top;
    canvas.drawText(text, 2,
                    - (pHeight - textHeight) / 2 + pHeight,
                    m_textPaint);

    return b;
  }

  @Override
  protected void onDraw(Canvas canvas)
  {
    Bookmark bmk = m_bmk;
    if (bmk != null)
    {
      int pinHeight = 35;
      m_popupAnchor.x = bmk.getPosition().x - m_width / 2;
      m_popupAnchor.y = bmk.getPosition().y - pinHeight - m_thriangleHeight - m_height;

      m_popupRect.left = m_popupAnchor.x;
      m_popupRect.top = m_popupAnchor.y;
      m_popupRect.right = m_popupAnchor.x + m_width;
      m_popupRect.bottom = m_popupAnchor.y + m_height;

      if (m_popup != null)
      {
        canvas.drawBitmap(m_popup, m_popupAnchor.x, m_popupAnchor.y, m_buttonPaint);
      }
    }
    else
    {
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
  public synchronized boolean handleClick(int x, int y)
  {
    if (m_bmk != null)
      if ( m_popupRect.contains(x, y) )
      {
        if (m_bmk.isPreviewBookmark())
        {
          //m_bmk.setCategoryId(BookmarkManager.getBookmarkManager(getContext()).getCategoriesCount()-1);
          getContext().startActivity(new Intent(getContext(), BookmarkActivity.class).
                        putExtra(BookmarkActivity.BOOKMARK_POSITION, new ParcelablePoint(m_bmk.getPosition())).
                        putExtra(BookmarkActivity.BOOKMARK_NAME, m_bmk.getName()));
        }
        else
        {
          getContext().startActivity(new Intent(getContext(), BookmarkActivity.class).putExtra(BookmarkActivity.PIN, new ParcelablePoint(m_bmk.getCategoryId(), m_bmk.getBookmarkId())));
        }
        m_bmk = null;
        return true;
      }
    return false;
  }


}
