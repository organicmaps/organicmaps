package com.mapswithme.maps.base;

import android.app.Application;
import android.content.Context;
import android.media.MediaPlayer;
import android.os.AsyncTask;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.RawRes;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;

public class MediaPlayerWrapper
{
  private static final int UNDEFINED_SOUND_STREAM = -1;

  @NonNull
  private final Application mApp;
  @Nullable
  private MediaPlayer mPlayer;
  @Nullable
  private MediaPlayer.OnCompletionListener mCompletionListener;
  private int mStreamResId = UNDEFINED_SOUND_STREAM;

  public MediaPlayerWrapper(@NonNull Application application)
  {
    mApp = application;
  }

  private boolean isCurrentSoundStream(@RawRes int streamResId)
  {
    return mStreamResId == streamResId;
  }

  @NonNull
  private Application getApp()
  {
    return mApp;
  }

  private void onInitializationCompleted(@NonNull InitializationResult initializationResult)
  {
    releaseInternal();
    mStreamResId = initializationResult.getStreamResId();
    mPlayer = initializationResult.getPlayer();
    if (mPlayer == null)
      return;

    mPlayer.setOnCompletionListener(mCompletionListener);
    mPlayer.start();
  }

  private void releaseInternal()
  {
    if (mPlayer == null)
      return;

    stop();
    mPlayer.release();
    mPlayer = null;
    mStreamResId = UNDEFINED_SOUND_STREAM;
  }

  @NonNull
  private static AsyncTask<Integer, Void, InitializationResult> makeInitTask(@NonNull MediaPlayerWrapper wrapper)
  {
    return new InitPlayerTask(wrapper);
  }

  public void release()
  {
    releaseInternal();
    mCompletionListener = null;
  }

  public void playback(@RawRes int streamResId,
                       @Nullable MediaPlayer.OnCompletionListener completionListener)
  {
    if (isCurrentSoundStream(streamResId) && mPlayer == null)
      return;

    if (isCurrentSoundStream(streamResId) && mPlayer != null)
    {
      mPlayer.start();
      return;
    }

    mCompletionListener = completionListener;
    mStreamResId = streamResId;
    AsyncTask<Integer, Void, InitializationResult> task = makeInitTask(this);
    task.execute(streamResId);
  }

  public void stop()
  {
    if (mPlayer == null)
      return;

    mPlayer.stop();
  }

  public boolean isPlaying()
  {
    return mPlayer != null && mPlayer.isPlaying();
  }

  @NonNull
  public static MediaPlayerWrapper from(@NonNull Context context)
  {
    MwmApplication app = (MwmApplication) context.getApplicationContext();
    return app.getMediaPlayer();
  }

  private static class InitPlayerTask extends AsyncTask<Integer, Void, InitializationResult>
  {
    @NonNull
    private final MediaPlayerWrapper mWrapper;

    InitPlayerTask(@NonNull MediaPlayerWrapper wrapper)
    {
      mWrapper = wrapper;
    }

    @Override
    protected InitializationResult doInBackground(Integer... params)
    {
      if (params.length == 0)
        throw new IllegalArgumentException("Params not found");
      int resId = params[0];
      MediaPlayer player = MediaPlayer.create(mWrapper.getApp(), resId);
      return new InitializationResult(player, resId);
    }

    @Override
    protected void onPostExecute(InitializationResult initializationResult)
    {
      super.onPostExecute(initializationResult);
      mWrapper.onInitializationCompleted(initializationResult);
    }
  }

  private static class InitializationResult
  {
    @Nullable
    private final MediaPlayer mPlayer;
    @RawRes
    private final int mStreamResId;

    private InitializationResult(@Nullable MediaPlayer player, @RawRes int streamResId)
    {
      mPlayer = player;
      mStreamResId = streamResId;
    }

    @RawRes
    private int getStreamResId()
    {
      return mStreamResId;
    }

    @Nullable
    private MediaPlayer getPlayer()
    {
      return mPlayer;
    }
  }
}
