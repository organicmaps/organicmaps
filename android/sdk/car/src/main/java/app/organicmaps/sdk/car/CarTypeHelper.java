package app.organicmaps.sdk.car;

import java.util.Objects;
import org.jspecify.annotations.NonNull;
import org.jspecify.annotations.Nullable;

public final class CarTypeHelper
{
  @Nullable
  static CarType sCarType;

  @NonNull
  public static CarType getCarType()
  {
    return Objects.requireNonNull(sCarType, "Car type is not initialized");
  }

  public static void setCarType(@Nullable CarType carType)
  {
    if (sCarType != null && sCarType != carType)
      throw new IllegalStateException("Car type is already initialized to " + sCarType + ", cannot change to "
                                      + carType);
    sCarType = carType;
  }

  private CarTypeHelper()
  {
    throw new RuntimeException();
  }
}
