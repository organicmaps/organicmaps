package com.mapswithme.maps.maplayer.guides;

import android.app.Application;
import android.content.Context;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.base.Detachable;
import com.mapswithme.maps.base.Initializable;
import com.mapswithme.maps.guides.GuidesGallery;

import java.util.ArrayList;
import java.util.List;

public class GuidesManager implements Initializable<Void>, Detachable<GuidesErrorDialogListener>
{
  @NonNull
  private final OnGuidesChangedListener mListener;
  @NonNull
  private final Application mApplication;
  @NonNull
  private final List<OnGuidesGalleryChangedListener> mGalleryChangedListeners
      = new ArrayList<>();

  @Nullable
  private GuidesErrorDialogListener mGuidesDialogListener;

  public GuidesManager(@NonNull Application application)
  {
    mApplication = application;
    mListener = this::onStateChanged;
  }

  private void onStateChanged(int index)
  {
    GuidesState state = GuidesState.values()[index];
    if (mGuidesDialogListener == null)
      state.activate(mApplication, null, null);
    else
      mGuidesDialogListener.onStateChanged(state);
  }

  public boolean isEnabled()
  {
    return Framework.nativeIsGuidesLayerEnabled();
  }

  public void addGalleryChangedListener(@NonNull OnGuidesGalleryChangedListener listener)
  {
    mGalleryChangedListeners.add(listener);
  }

  public void removeGalleryChangedListener(@NonNull OnGuidesGalleryChangedListener listener)
  {
    mGalleryChangedListeners.remove(listener);
  }

  public void setEnabled(boolean isEnabled)
  {
    if (isEnabled == isEnabled())
      return;

    Framework.nativeSetGuidesLayerEnabled(isEnabled);
  }

  public void toggle()
  {
    setEnabled(!isEnabled());
  }

  @Override
  public void initialize(@Nullable Void param)
  {
    nativeSetGuidesStateChangedListener(mListener);
    nativeSetGalleryChangedListener();
  }

  @Override
  public void destroy()
  {
    nativeRemoveGuidesStateChangedListener();
    nativeRemoveGalleryChangedListener();
  }

  public void setActiveGuide(@NonNull String guideId)
  {
    nativeSetActiveGuide(guideId);
  }

  @NonNull
  public String getActiveGuide()
  {
    return nativeGetActiveGuide();
  }

  @NonNull
  public GuidesGallery getGallery()
  {
    return nativeGetGallery();
  }

  @Override
  public void attach(@NonNull GuidesErrorDialogListener listener)
  {
    mGuidesDialogListener = listener;
  }

  @Override
  public void detach()
  {
    mGuidesDialogListener = null;
  }

  @NonNull
  public static GuidesManager from(@NonNull Context context)
  {
    MwmApplication app = (MwmApplication) context.getApplicationContext();
    return app.getGuidesManager();
  }

  // Called from JNI.
  public void onGalleryChanged(boolean reloadGallery)
  {
    for (OnGuidesGalleryChangedListener listener : mGalleryChangedListeners)
      listener.onGuidesGalleryChanged(reloadGallery);
  }

  private static native void nativeSetGuidesStateChangedListener(@NonNull OnGuidesChangedListener listener);
  private static native void nativeRemoveGuidesStateChangedListener();
  private static native void nativeSetGalleryChangedListener();
  private static native void nativeRemoveGalleryChangedListener();
  private static native void nativeSetActiveGuide(@NonNull String guideId);
  @NonNull
  private static native String nativeGetActiveGuide();
  @NonNull
  private static native GuidesGallery nativeGetGallery();
}
