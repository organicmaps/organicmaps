package com.mapswithme.maps.pins;

import java.util.List;

import android.content.Context;
import android.widget.ArrayAdapter;

import com.mapswithme.maps.pins.pins.PinSet;

public class AbstractPinSetAdapter extends ArrayAdapter<PinSet>
{

  public AbstractPinSetAdapter(Context context, List<PinSet> objects)
  {
    super(context, 0, 0, objects);
  }

}
