package app.organicmaps.intent;

import android.content.Intent;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public interface IntentProcessor
{
  @Nullable
  MapTask process(@NonNull Intent intent);
}
