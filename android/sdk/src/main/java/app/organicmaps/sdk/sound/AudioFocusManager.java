package app.organicmaps.sdk.sound;

import android.content.Context;
import android.media.AudioAttributes;
import android.media.AudioFocusRequest;
import android.media.AudioManager;
import android.os.Build;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public class AudioFocusManager
{
  @NonNull
  public static final AudioAttributes AUDIO_ATTRIBUTES =
      new AudioAttributes.Builder()
          .setUsage(AudioAttributes.USAGE_ASSISTANCE_NAVIGATION_GUIDANCE)
          .setContentType(AudioAttributes.CONTENT_TYPE_SPEECH)
          .build();

  @Nullable
  private AudioManager mAudioManager = null;
  @Nullable
  private AudioManager.OnAudioFocusChangeListener mOnFocusChange = null;
  @Nullable
  private AudioFocusRequest mAudioFocusRequest = null;

  private int mFocusMode;

  public AudioFocusManager(@Nullable Context context, boolean silenceMusic)
  {
    if (context != null)
      mAudioManager = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);

    init(silenceMusic);
  }

  public void setSilenceMusic(boolean silenceMusic)
  {
    init(silenceMusic);
  }

  public boolean requestAudioFocus()
  {
    boolean isMusicActive = false;

    if (mAudioManager != null)
    {
      isMusicActive = mAudioManager.isMusicActive();
      if (Build.VERSION.SDK_INT < 26)
        mAudioManager.requestAudioFocus(mOnFocusChange, AudioManager.STREAM_VOICE_CALL, mFocusMode);
      else
        mAudioManager.requestAudioFocus(mAudioFocusRequest);
    }

    return isMusicActive;
  }

  public void releaseAudioFocus()
  {
    if (mAudioManager != null)
    {
      if (Build.VERSION.SDK_INT < 26)
        mAudioManager.abandonAudioFocus(mOnFocusChange);
      else
        mAudioManager.abandonAudioFocusRequest(mAudioFocusRequest);
    }
  }

  public void init(boolean silenceMusic)
  {
    mFocusMode = silenceMusic ? AudioManager.AUDIOFOCUS_GAIN_TRANSIENT_EXCLUSIVE
                              : AudioManager.AUDIOFOCUS_GAIN_TRANSIENT_MAY_DUCK;

    if (Build.VERSION.SDK_INT < 26)
      mOnFocusChange = focusGain -> {};
    else
      mAudioFocusRequest = new AudioFocusRequest.Builder(mFocusMode).setAudioAttributes(AUDIO_ATTRIBUTES).build();
  }
}
