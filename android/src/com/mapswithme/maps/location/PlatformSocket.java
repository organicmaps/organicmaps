package com.mapswithme.maps.location;

import android.support.annotation.NonNull;

interface PlatformSocket
{
  boolean open(@NonNull String host, int port);

  void close();

  void setTimeout(int millis);

  boolean read(@NonNull byte[] data, int count);

  boolean write(@NonNull byte[] data, int count);
}
