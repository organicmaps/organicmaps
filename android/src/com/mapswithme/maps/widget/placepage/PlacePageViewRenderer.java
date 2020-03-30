package com.mapswithme.maps.widget.placepage;

import android.os.Bundle;
import android.view.View;

import androidx.annotation.NonNull;
import com.mapswithme.maps.base.Hideable;
import com.mapswithme.maps.base.Initializable;
import com.mapswithme.maps.base.Savable;

public interface PlacePageViewRenderer<Data> extends Initializable<View>, Savable<Bundle>,
                                                     Hideable
{
  void render(@NonNull Data data);
}
