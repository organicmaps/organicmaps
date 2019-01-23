package com.mapswithme.maps.dialog;

import android.app.Dialog;
import android.os.Bundle;
import android.support.annotation.NonNull;

public interface ResolveDialogViewStrategy
{
  @NonNull
  Dialog createView(@NonNull AlertDialog dialog, @NonNull Bundle args);
}
