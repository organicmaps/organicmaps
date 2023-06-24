package app.organicmaps.widget.placepage;

import android.os.Bundle;
import android.view.View;

import androidx.annotation.NonNull;

import app.organicmaps.base.Hideable;
import app.organicmaps.base.Initializable;
import app.organicmaps.base.Savable;
import app.organicmaps.base.Supportable;

public interface PlacePageViewRenderer<Data> extends Initializable<View>, Savable<Bundle>,
                                                     Hideable, Supportable<Data>
{
  void render(@NonNull Data data);
}
