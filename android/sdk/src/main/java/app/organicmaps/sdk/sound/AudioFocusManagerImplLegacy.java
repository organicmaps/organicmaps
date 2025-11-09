package app.organicmaps.sdk.sound;

import static android.media.AudioManager.AUDIOFOCUS_GAIN_TRANSIENT_MAY_DUCK;

import android.content.Context;
import android.media.AudioManager;
import androidx.annotation.NonNull;
import app.organicmaps.sdk.util.log.Logger;

final class AudioFocusManagerImplLegacy extends AudioFocusManager
{
  private static final String TAG = AudioFocusManagerImpl.class.getSimpleName();

  public AudioFocusManagerImplLegacy(@NonNull Context context, @NonNull OnAudioFocusLost onAudioFocusLost)
  {
    super(context, onAudioFocusLost);
  }

  public boolean requestAudioFocus()
  {
    final int requestResult = mAudioManager.requestAudioFocus(this::onAudioFocusChange, AudioManager.STREAM_VOICE_CALL,
                                                              AUDIOFOCUS_GAIN_TRANSIENT_MAY_DUCK);
    mPlaybackAllowed = requestResult == AudioManager.AUDIOFOCUS_REQUEST_GRANTED;
    if (!mPlaybackAllowed)
      Logger.w(TAG, "Audio focus request failed");
    return mPlaybackAllowed;
  }

  public void releaseAudioFocus()
  {
    mAudioManager.abandonAudioFocus(this::onAudioFocusChange);
    mPlaybackAllowed = false;
  }
}
