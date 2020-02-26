package com.mapswithme.maps.widget.placepage;

import android.view.View;

import androidx.annotation.NonNull;
import com.mapswithme.maps.base.Initializable;

public interface PlacePageViewRenderer<Data> extends Initializable<View>
{
  void render(@NonNull Data data);
}
