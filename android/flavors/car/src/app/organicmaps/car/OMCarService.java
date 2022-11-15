package app.organicmaps.car;

import androidx.annotation.NonNull;
import androidx.car.app.CarAppService;
import androidx.car.app.validation.HostValidator;

public final class OMCarService extends CarAppService
{
  @NonNull
  @Override
  public HostValidator createHostValidator()
  {
    throw new UnsupportedOperationException("Not implemented, yet");
  }
}
