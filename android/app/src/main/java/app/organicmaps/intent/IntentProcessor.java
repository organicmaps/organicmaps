package app.organicmaps.intent;

import android.content.Intent;
import androidx.annotation.NonNull;
import app.organicmaps.MwmActivity;

public interface IntentProcessor
{
  /** @return true if the intent was handled and no further processors should run. */
  boolean process(@NonNull Intent intent, @NonNull MwmActivity activity);
}
