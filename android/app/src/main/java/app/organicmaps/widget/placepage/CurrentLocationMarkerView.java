package app.organicmaps.widget.placepage;

import android.annotation.SuppressLint;
import android.content.Context;
import androidx.annotation.NonNull;
import app.organicmaps.R;
import com.github.mikephil.charting.components.MarkerView;
import com.github.mikephil.charting.utils.MPPointF;

@SuppressLint("ViewConstructor")
public class CurrentLocationMarkerView extends MarkerView
{
  /**
   * Constructor. Sets up the MarkerView with a custom layout resource.
   *
   * @param context
   */
  public CurrentLocationMarkerView(@NonNull Context context)
  {
    super(context, R.layout.current_location_marker);
  }

  @Override
  public MPPointF getOffset()
  {
    return new MPPointF(-(getWidth() / 2f), -getHeight());
  }

  @Override
  public MPPointF getOffsetForDrawingAtPoint(float posX, float posY)
  {
    return getOffset();
  }
}
