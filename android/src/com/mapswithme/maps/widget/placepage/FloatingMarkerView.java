
package com.mapswithme.maps.widget.placepage;

import android.annotation.SuppressLint;
import android.content.Context;
import android.view.View;
import android.widget.RelativeLayout;
import android.widget.TextView;

import androidx.annotation.NonNull;
import com.github.mikephil.charting.components.MarkerView;
import com.github.mikephil.charting.data.Entry;
import com.github.mikephil.charting.highlight.Highlight;
import com.github.mikephil.charting.utils.MPPointF;
import com.mapswithme.maps.R;
import com.mapswithme.util.StringUtils;

@SuppressLint("ViewConstructor")
public class FloatingMarkerView extends MarkerView
{
  private static final int TRIANGLE_ROTATION_ANGLE = 180;
  @NonNull
  private final View mImage;
  @NonNull
  private final View mInfoFloatingContainer;
  @NonNull
  private final View mSlidingContainer;
  @NonNull
  private final TextView mAltitudeView;
  @NonNull
  private final TextView mDistanceTextView;
  @NonNull
  private final TextView mDistanceValueView;
  @NonNull
  private final View mFloatingTriangle;
  @NonNull
  private final View mTextContentContainer;

  private float mOffset;

  public FloatingMarkerView(@NonNull Context context)
  {
    super(context, R.layout.floating_marker_view);
    mInfoFloatingContainer = findViewById(R.id.info_floating_container);
    mTextContentContainer = findViewById(R.id.floating_text_container);
    mFloatingTriangle = findViewById(R.id.floating_triangle);
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
    final float halfImg = Math.abs(mImage.getWidth()) / 2f;
    boolean isLeftToRightDirection = isInvertedOrder(highlight);
    mOffset = isLeftToRightDirection ? -getWidth() + halfImg : -halfImg;
    updateHorizontal(highlight);
  }

  private boolean isInvertedOrder(@NonNull Highlight highlight)
  {
    float x = highlight.getX();
    float halfImg = Math.abs(mImage.getWidth()) / 2f;
    int wholeText = Math.abs(mInfoFloatingContainer.getWidth());
    float factor = calcHorizontalFactor();
    return x + (halfImg + wholeText ) * factor >= getChartView().getXChartMax();
  }

  private float calcHorizontalFactor() {
    float delta = getChartView().getXChartMax() - getChartView().getXChartMin();
    return delta / getChartView().getContentRect().width();
  }

  private float convertContainerHeight()
  {
    float height = getChartView().getContentRect().height();
    float delta = getChartView().getYMax() - getChartView().getYMin();
    float factor =  delta / height;
    return factor * mSlidingContainer.getHeight();
  }

  private void updateVertical(@NonNull Entry entry)
  {
    LayoutParams layoutParams = (LayoutParams) mTextContentContainer.getLayoutParams();
    float posY = entry.getY();
    float halfContent = convertContainerHeight() / 2f;

    if (posY + halfContent >= getChartView().getYChartMax())
    {
      layoutParams.addRule(ALIGN_PARENT_BOTTOM);
      layoutParams.removeRule(ALIGN_PARENT_TOP);
      layoutParams.removeRule(CENTER_VERTICAL);
    }
    else if (posY - halfContent <= getChartView().getYChartMin())
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
    mTextContentContainer.setLayoutParams(layoutParams);
  }

  private void updatePointValues(@NonNull Entry entry)
  {
    mDistanceTextView.setText(R.string.elevation_profile_distance);
    mDistanceValueView.setText(StringUtils.nativeFormatDistance(entry.getX()));
    mAltitudeView.setText(StringUtils.nativeFormatDistance(entry.getY()));
  }

  private void updateHorizontal(@NonNull Highlight highlight)
  {
    LayoutParams textParams = (LayoutParams) mInfoFloatingContainer.getLayoutParams();
    LayoutParams imgParams = (LayoutParams) mImage.getLayoutParams();

    boolean isInvertedOrder = isInvertedOrder(highlight);
    int anchorId = isInvertedOrder ? mInfoFloatingContainer.getId() : mImage.getId();
    LayoutParams toBecomeAnchor = isInvertedOrder ? textParams : imgParams;
    LayoutParams toBecomeDependent = isInvertedOrder ? imgParams : textParams;

    toBecomeAnchor.removeRule(RelativeLayout.END_OF);
    toBecomeAnchor.removeRule(RelativeLayout.RIGHT_OF);
    toBecomeDependent.addRule(RelativeLayout.END_OF, anchorId);

    mFloatingTriangle.setRotation(isInvertedOrder ? 0 : TRIANGLE_ROTATION_ANGLE);
    mInfoFloatingContainer.setLayoutParams(textParams);
    mImage.setLayoutParams(imgParams);

    LayoutParams triangleParams = (LayoutParams) mFloatingTriangle.getLayoutParams();
    LayoutParams textContentParams = (LayoutParams) mTextContentContainer.getLayoutParams();

    toBecomeAnchor = isInvertedOrder ? textContentParams : triangleParams;
    toBecomeDependent = isInvertedOrder ? triangleParams : textContentParams;
    anchorId = isInvertedOrder ? mTextContentContainer.getId() : mFloatingTriangle.getId();

    toBecomeAnchor.removeRule(RelativeLayout.END_OF);
    toBecomeAnchor.removeRule(RelativeLayout.RIGHT_OF);
    toBecomeDependent.addRule(END_OF, anchorId);

    mFloatingTriangle.setLayoutParams(triangleParams);
    mTextContentContainer.setLayoutParams(textContentParams);
  }
}
