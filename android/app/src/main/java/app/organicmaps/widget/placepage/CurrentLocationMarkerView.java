package app.organicmaps.widget.placepage;

import android.annotation.SuppressLint;
import android.content.Context;
import android.view.View;
import androidx.annotation.NonNull;
import app.organicmaps.R;
import com.github.mikephil.charting.components.MarkerView;
import com.github.mikephil.charting.utils.MPPointF;

@SuppressLint("ViewConstructor")
public class CurrentLocationMarkerView extends MarkerView
{
  @NonNull
  private final View mLabel;
  @NonNull
  private final View mPin;

  /**
   * Constructor. Sets up the MarkerView with a custom layout resource.
   *
   * @param context
   */
  public CurrentLocationMarkerView(@NonNull Context context)
  {
    super(context, R.layout.current_location_marker);
    // The children are repositioned below, let them leave the marker's bounds.
    setClipChildren(false);
    mLabel = findViewById(R.id.label);
    mPin = findViewById(R.id.image);
  }

  @Override
  public MPPointF getOffset()
  {
    return new MPPointF(-(getWidth() / 2f), -getHeight());
  }

  @Override
  public MPPointF getOffsetForDrawingAtPoint(float posX, float posY)
  {
    // Keeps the label from being cut off at the chart's edges. The base class' bottom clamp never
    // triggers: the marker always sits exactly one height above the point.
    MPPointF offset = super.getOffsetForDrawingAtPoint(posX, posY);
    mPin.setTranslationX(-(offset.x + mPin.getLeft() + mPin.getWidth() / 2f));
    // The shift moved the pin away from the point it marks, put it back. Near the top of the chart
    // that leaves the pin on top of the label, so move the label under the pin instead.
    float shift = -(offset.y + mPin.getBottom());
    mPin.setTranslationY(shift);
    mLabel.setTranslationY(shift < 0f ? mPin.getBottom() + shift : 0f);
    return offset;
  }
}
