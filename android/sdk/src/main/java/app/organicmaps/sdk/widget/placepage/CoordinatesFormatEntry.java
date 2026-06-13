package app.organicmaps.sdk.widget.placepage;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

/// One coordinate format resolved at a point: its stable id (place_page::CoordinatesFormat) plus the
/// labelled display string for the place-page row ("OSGB: SW 7400 4210") and the bare value for copying.
public final class CoordinatesFormatEntry
{
  private final int mId;
  @NonNull
  private final String mDisplay;
  @NonNull
  private final String mValue;

  // Used by JNI.
  @Keep
  public CoordinatesFormatEntry(int id, @NonNull String display, @NonNull String value)
  {
    mId = id;
    mDisplay = display;
    mValue = value;
  }

  public int getId()
  {
    return mId;
  }

  @NonNull
  public String getDisplay()
  {
    return mDisplay;
  }

  @NonNull
  public String getValue()
  {
    return mValue;
  }
}
