package app.organicmaps.sdk.sound;

import android.content.Context;
import android.media.AudioAttributes;
import android.media.AudioManager;
import androidx.annotation.NonNull;

abstract class AudioFocusManager
{
  public interface OnAudioFocusLost
  {
    void onAudioFocusLost();
  }

  public static final AudioAttributes AUDIO_ATTRIBUTES =
      new AudioAttributes.Builder()
          .setUsage(AudioAttributes.USAGE_ASSISTANCE_NAVIGATION_GUIDANCE)
          .setContentType(AudioAttributes.CONTENT_TYPE_SPEECH)
          .build();

  @NonNull
  protected final AudioManager mAudioManager;
  @NonNull
  private final OnAudioFocusLost mOnAudioFocusLost;
  protected boolean mPlaybackAllowed = false;

  protected AudioFocusManager(@NonNull Context context, @NonNull OnAudioFocusLost onAudioFocusLost)
  {
    mAudioManager = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
    mOnAudioFocusLost = onAudioFocusLost;
  }

  @NonNull
  public static AudioFocusManager create(@NonNull Context context, @NonNull OnAudioFocusLost onAudioFocusLost)
  {
    if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.O)
      return new AudioFocusManagerImpl(context, onAudioFocusLost);
    else
      return new AudioFocusManagerImplLegacy(context, onAudioFocusLost);
  }

  public abstract boolean requestAudioFocus();

  public abstract void releaseAudioFocus();

  public boolean isMusicActive()
  {
    return mAudioManager.isMusicActive();
  }

  protected void onAudioFocusChange(int focusChange)
  {
    if (focusChange == AudioManager.AUDIOFOCUS_GAIN || focusChange == AudioManager.AUDIOFOCUS_GAIN_TRANSIENT
        || focusChange == AudioManager.AUDIOFOCUS_GAIN_TRANSIENT_MAY_DUCK)
    {
      mPlaybackAllowed = true;
    }
    else if (focusChange == AudioManager.AUDIOFOCUS_LOSS || focusChange == AudioManager.AUDIOFOCUS_LOSS_TRANSIENT
             || focusChange == AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK)
    {
      mPlaybackAllowed = false;
      mOnAudioFocusLost.onAudioFocusLost();
    }
  }
}
