package app.organicmaps.sdk.sound;

import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.media.MediaPlayer;
import androidx.annotation.NonNull;
import androidx.annotation.RawRes;
import app.organicmaps.sdk.util.log.Logger;
import java.io.IOException;

public class MediaPlayerWrapper
{
  private static final String TAG = MediaPlayerWrapper.class.getSimpleName();
  @RawRes
  private static final int UNDEFINED_SOUND_STREAM = 0;

  @NonNull
  private final Context mContext;
  @NonNull
  private final MediaPlayer mPlayer;

  private boolean mIsInitialized = false;
  @RawRes
  private int mStreamResId = UNDEFINED_SOUND_STREAM;

  public MediaPlayerWrapper(@NonNull Context context)
  {
    if (android.os.Build.VERSION.SDK_INT >= 30)
      mContext = context.createAttributionContext("media_playback");
    else
      mContext = context;
    mPlayer = new MediaPlayer();
    mPlayer.setAudioAttributes(AudioFocusManager.AUDIO_ATTRIBUTES);
    mPlayer.setOnPreparedListener(this::onPrepared);
  }

  public void release()
  {
    stop();
    mPlayer.release();
    mStreamResId = UNDEFINED_SOUND_STREAM;
  }

  public void playback(@RawRes int streamResId)
  {
    if (isInitialized() && isCurrentSoundStream(streamResId))
    {
      mPlayer.start();
      return;
    }

    initialize(streamResId);
  }

  public void stop()
  {
    mPlayer.stop();
  }

  private void initialize(@RawRes int streamResId)
  {
    mIsInitialized = false;
    mPlayer.reset();
    try (final AssetFileDescriptor afd = mContext.getResources().openRawResourceFd(streamResId))
    {
      if (afd == null)
        return;
      mPlayer.setDataSource(afd.getFileDescriptor(), afd.getStartOffset(), afd.getLength());
      mPlayer.prepareAsync();
      mStreamResId = streamResId;
    }
    catch (IllegalStateException e)
    {
      Logger.w(TAG, "MediaPlayer illegal state while initializing", e);
    }
    catch (IllegalArgumentException e)
    {
      Logger.w(TAG, "AssetFileDescriptor is not a valid FileDescriptor", e);
    }
    catch (IOException e)
    {
      Logger.w(TAG, "AssetFileDescriptor cannot be read", e);
    }
    finally
    {
      mIsInitialized = false;
    }
  }

  private boolean isCurrentSoundStream(@RawRes int streamResId)
  {
    return mStreamResId == streamResId;
  }

  private boolean isInitialized()
  {
    return mIsInitialized;
  }

  private void onPrepared(@NonNull MediaPlayer unused)
  {
    mIsInitialized = true;
    mPlayer.start();
  }
}
