package com.mapswithme.maps.auth;

import android.content.Intent;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.mapswithme.maps.Framework;

interface TokenHandler
{
  boolean checkToken(int requestCode, @NonNull Intent data);

  @Nullable
  String getToken();

  @Framework.AuthTokenType
  int getType();
}
