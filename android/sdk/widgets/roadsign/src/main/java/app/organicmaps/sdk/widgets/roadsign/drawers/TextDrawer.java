package app.organicmaps.sdk.widgets.roadsign.drawers;

import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.Typeface;
import androidx.annotation.NonNull;
import androidx.annotation.RestrictTo;
import androidx.annotation.RestrictTo.Scope;

@RestrictTo(Scope.LIBRARY)
public final class TextDrawer extends ContentDrawer
{
  @NonNull
  private String mText = "";

  public TextDrawer()
  {
    super();
    mPaint.setTextAlign(Paint.Align.CENTER);
    mPaint.setTypeface(Typeface.create(Typeface.DEFAULT, Typeface.BOLD));
  }

  public void setText(@NonNull String text)
  {
    mText = text;
    configureTextSize();
  }

  @Override
  public void onSizeChanged(float radiusBound)
  {
    super.onSizeChanged(radiusBound);
    configureTextSize();
  }

  public void draw(@NonNull Canvas canvas, float cx, float cy)
  {
    mPaint.setColor(mConfig.color);

    final Rect textBounds = new Rect();
    mPaint.getTextBounds(mText, 0, mText.length(), textBounds);
    final float textY = cy - textBounds.exactCenterY();
    canvas.drawText(mText, cx, textY, mPaint);
  }

  // Apply binary search to determine the optimal text size that fits within the circular boundary.
  private void configureTextSize()
  {
    final float textRadius = mRadiusBound;
    final float textMaxSize = 2 * textRadius;
    final float textMaxSizeSquared = (float) Math.pow(textMaxSize, 2);

    float lowerBound = 0;
    float upperBound = textMaxSize;
    float textSize = textMaxSize;
    final Rect textBounds = new Rect();

    while (lowerBound <= upperBound)
    {
      textSize = (lowerBound + upperBound) / 2;
      mPaint.setTextSize(textSize);
      mPaint.getTextBounds(mText, 0, mText.length(), textBounds);

      if (Math.pow(textBounds.width(), 2) + Math.pow(textBounds.height(), 2) <= textMaxSizeSquared)
        lowerBound = textSize + 1;
      else
        upperBound = textSize - 1;
    }

    mPaint.setTextSize(Math.max(1, textSize));
  }
}
