package com.mapswithme.maps.auth;

import android.support.annotation.Nullable;

import com.mapswithme.maps.Framework;

interface TokenHandler
{
  boolean checkToken();

  @Nullable
  String getToken();

  @Framework.AuthTokenType
  int getType();
}
