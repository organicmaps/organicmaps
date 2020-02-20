
package com.mapswithme.maps.widget.placepage;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.RectF;
import android.view.View;
import android.widget.RelativeLayout;
import android.widget.TextView;

import androidx.annotation.NonNull;
import com.github.mikephil.charting.components.MarkerView;
import com.github.mikephil.charting.data.Entry;
import com.github.mikephil.charting.highlight.Highlight;
import com.github.mikephil.charting.utils.MPPointF;
import com.mapswithme.maps.R;

import java.util.Locale;

@SuppressLint("ViewConstructor")
public class FloatingMarkerView extends MarkerView
{
  @NonNull
  private final View mImage;
  @NonNull
  private final View mTextContainer;
  @NonNull
  private final View mSlidingContainer;
  @NonNull
  private final TextView mAltitudeView;
  @NonNull
  private final TextView mDistanceTextView;
  @NonNull
  private final TextView mDistanceValueView;

  private float mOffset;

  public FloatingMarkerView(@NonNull Context context)
  {
    super(context, R.layout.floating_marker_view);
    mTextContainer = findViewById(R.id.text_container);
    mImage = findViewById(R.id.image);
    mSlidingContainer = findViewById(R.id.sliding_container);
    mDistanceTextView = findViewById(R.id.distance_text);
    mAltitudeView = findViewById(R.id.altitude);
    mDistanceValueView = findViewById(R.id.distance_value);
  }

  // runs every time the MarkerView is redrawn, can be used to update the
  // content (user-interface)
  @Override
  public void refreshContent(Entry e, Highlight highlight)
  {
    updatePointValues(e);

    super.refreshContent(e, highlight);
  }

  @Override
  public MPPointF getOffset()
  {
    return new MPPointF(mOffset, -getHeight() / 2f);
  }

  @Override
  public MPPointF getOffsetForDrawingAtPoint(float posX, float posY)
  {
    return getOffset();
  }

  public void updateOffsets(@NonNull Entry entry, @NonNull Highlight highlight)
  {
    updateVertical(entry);
    updateHorizontal(highlight);
  }

  private void updateVertical(@NonNull Entry entry)
  {
    LayoutParams layoutParams = (LayoutParams) mTextContainer.getLayoutParams();
    float posY = entry.getY();
    if (posY + mSlidingContainer.getHeight() / 2f >= getChartView().getYChartMax())
    {
      layoutParams.addRule(ALIGN_PARENT_BOTTOM);
      layoutParams.removeRule(ALIGN_PARENT_TOP);
      layoutParams.removeRule(CENTER_VERTICAL);
    }
    else if (posY - mSlidingContainer.getHeight() / 2f <= getChartView().getYChartMin())
    {
      layoutParams.addRule(ALIGN_PARENT_TOP);
      layoutParams.removeRule(ALIGN_PARENT_BOTTOM);
      layoutParams.removeRule(CENTER_VERTICAL);
    }
    else
    {
      layoutParams.addRule(CENTER_VERTICAL);
      layoutParams.removeRule(ALIGN_PARENT_TOP);
      layoutParams.removeRule(ALIGN_PARENT_BOTTOM);
    }
    mTextContainer.setLayoutParams(layoutParams);
  }

  private void updatePointValues(@NonNull Entry entry)
  {
    mDistanceTextView.setText("Distance : ");
    mDistanceValueView.setText(String.format("%s km", entry.getX()));
    mAltitudeView.setText(String.format(Locale.US, "%.2fm", entry.getY()));
  }

  private void updateHorizontal(@NonNull Highlight highlight)
  {
    final float halfImg = Math.abs(mImage.getWidth()) / 2f;
    final int wholeText = Math.abs(mTextContainer.getWidth());
    RectF rect = getChartView().getContentRect();

    LayoutParams textParams = (LayoutParams) mTextContainer.getLayoutParams();
    LayoutParams imgParams = (LayoutParams) mImage.getLayoutParams();

    boolean isLeftToRightDirection = highlight.getXPx() + halfImg + wholeText >= rect.right;
    mOffset = isLeftToRightDirection ? -getWidth() + halfImg : -halfImg;
    int anchorId = isLeftToRightDirection ? R.id.text_container : R.id.image;
    LayoutParams toBecomeAnchor = isLeftToRightDirection ? textParams : imgParams;
    LayoutParams toBecomeDependent = isLeftToRightDirection ? imgParams : textParams;

    toBecomeAnchor.removeRule(RelativeLayout.END_OF);
    toBecomeDependent.addRule(RelativeLayout.END_OF, anchorId);

    mTextContainer.setLayoutParams(textParams);
    mImage.setLayoutParams(imgParams);
  }
}
