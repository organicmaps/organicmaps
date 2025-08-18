package app.organicmaps.sdk.sync;

import java.util.Map;
import java.util.stream.Collectors;
import java.util.stream.Stream;

public enum BackendType implements SyncBackend
{
  ; // TODO add sync backends (PR #10651)

  public static final Map<Integer, BackendType> idToBackendType =
      Stream.of(BackendType.values()).collect(Collectors.toMap(BackendType::getId, b -> b));
  @Override
  public int getId()
  {
    throw new RuntimeException("STUB"); // TODO implement the method on a per-type basis (PR #10651)
  }
}
