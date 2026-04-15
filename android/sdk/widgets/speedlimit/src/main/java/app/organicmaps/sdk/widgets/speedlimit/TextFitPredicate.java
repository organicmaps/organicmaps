package app.organicmaps.sdk.widgets.speedlimit;

import android.graphics.RectF;
import androidx.annotation.NonNull;
import androidx.annotation.RestrictTo;

@RestrictTo(RestrictTo.Scope.LIBRARY)
interface TextFitPredicate {
  boolean fits(@NonNull RectF textBounds);
}
