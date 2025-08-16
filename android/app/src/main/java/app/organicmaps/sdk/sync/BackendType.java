package app.organicmaps.sdk.sync;

import java.util.Map;
import java.util.stream.Collectors;
import java.util.stream.Stream;

public enum BackendType implements SyncBackend
{
  Nextcloud {
    @Override
    public int getId()
    {
      return 0;
    }
  };

  public static final Map<Integer, BackendType> idToBackendType =
      Stream.of(BackendType.values()).collect(Collectors.toMap(BackendType::getId, b -> b));
}
