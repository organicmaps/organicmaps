package com.mapswithme.maps.bookmarks;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.ParcelablePoint;

import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
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
  private Bookmark m_bmk;
  private Paint m_backgroundPaint;
  private Paint m_borderPaint;
  private Paint m_buttonPaint = new Paint();
  private Path m_popupPath;
  private TextPaint m_textPaint;
  private Rect m_textBounds = new Rect();
  private Rect m_popupRect = new Rect();
  private Point m_popupAnchor = new Point();

  public PopupLayout(Context context)
  {
    this(context, null, 0);
    // TODO Auto-generated constructor stub
  }

  public PopupLayout(Context context, AttributeSet attrs)
  {
    this(context, attrs, 0);
    // TODO Auto-generated constructor stub
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
    m_textPaint.setTextSize(30);
    m_textPaint.setAntiAlias(true);
  }

  public synchronized void activate(final Bookmark bmk)
  {
    m_bmk = bmk;
    postInvalidate();
  }

  public synchronized void deactivate()
  {
    m_bmk = null;
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

  @Override
  protected void onDraw(Canvas canvas)
  {
    synchronized (this)
    {
      Bookmark bmk = m_bmk;
      if (isActive())
      {

        anchor = bmk.getPosition();

        int pinHeight = m_pin.getHeight();


        Bitmap btn;
        if (bmk.isPreviewBookmark())
        {
          btn = m_AddButton;
          canvas.drawBitmap(m_pin, anchor.x - m_pin.getWidth() / 2, anchor.y - pinHeight, m_borderPaint);
        }
        else
        {
          btn = m_editButton;
        }

        int pWidth = getWidth() / 2;
        int pHeight = btn.getHeight() + 10;

        m_popupAnchor.x = anchor.x - pWidth / 2;
        m_popupAnchor.y = anchor.y - pinHeight - m_thriangleHeight - pHeight;

        m_popupRect.left = m_popupAnchor.x;
        m_popupRect.top = m_popupAnchor.y;
        m_popupRect.right = m_popupAnchor.x + pWidth;
        m_popupRect.bottom = m_popupAnchor.y + pHeight;

        m_popupPath.reset();
        m_popupPath.moveTo(m_popupAnchor.x, m_popupAnchor.y);
        m_popupPath.lineTo(m_popupAnchor.x + pWidth, m_popupAnchor.y);
        m_popupPath.lineTo(m_popupAnchor.x + pWidth, m_popupAnchor.y + pHeight);
        m_popupPath.lineTo(anchor.x + m_thriangleHeight, m_popupAnchor.y + pHeight);
        m_popupPath.lineTo(anchor.x,  m_popupAnchor.y + pHeight + m_thriangleHeight);
        m_popupPath.lineTo(anchor.x - m_thriangleHeight,  m_popupAnchor.y + pHeight);
        m_popupPath.lineTo(m_popupAnchor.x,  m_popupAnchor.y + pHeight);
        m_popupPath.lineTo(m_popupAnchor.x, m_popupAnchor.y);

        String text = (String) TextUtils.ellipsize(bmk.getName(), m_textPaint, pWidth - btn.getWidth() - 10, TruncateAt.END);
        canvas.drawPath(m_popupPath, m_backgroundPaint);
        canvas.drawBitmap(btn, m_popupAnchor.x + pWidth - btn.getWidth()-5, m_popupAnchor.y + 5, m_buttonPaint);
        canvas.drawPath(m_popupPath, m_borderPaint);
        m_textPaint.getTextBounds(bmk.getName(), 0, bmk.getName().length(), m_textBounds);
        int textHeight = m_textBounds.bottom - m_textBounds.top;
        canvas.drawText(text, m_popupAnchor.x+2,
                        m_popupAnchor.y - (pHeight - textHeight) / 2 + pHeight,
                        m_textPaint);

      }
      else
      {
        super.onDraw(canvas);
      }
    }
  }

  public void handleClick(int x, int y)
  {
    if ( m_popupRect.contains(x, y) )
    {
      if (m_bmk.isPreviewBookmark())
      {
        getContext().startActivity(new Intent(getContext(), BookmarkActivity.class).
                      putExtra(BookmarkActivity.BOOKMARK_POSITION, new ParcelablePoint(m_bmk.getPosition())));
      }
      else
      {
        getContext().startActivity(new Intent(getContext(), BookmarkActivity.class).putExtra(BookmarkActivity.PIN, new ParcelablePoint(m_bmk.getCategoryId(), m_bmk.getBookmarkId())));
      }
      m_bmk = null;
    }
  }
}
