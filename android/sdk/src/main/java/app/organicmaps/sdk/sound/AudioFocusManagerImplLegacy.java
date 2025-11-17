package app.organicmaps.sdk.sound;

import static android.media.AudioManager.AUDIOFOCUS_GAIN_TRANSIENT_MAY_DUCK;

import android.content.Context;
import android.media.AudioManager;
import androidx.annotation.NonNull;

final class AudioFocusManagerImplLegacy extends AudioFocusManager
{
  public AudioFocusManagerImplLegacy(@NonNull Context context, @NonNull OnAudioFocusLost onAudioFocusLost)
  {
    super(context, onAudioFocusLost);
  }

  @Override
  protected boolean requestAudioFocusImpl()
  {
    final int requestResult = mAudioManager.requestAudioFocus(this::onAudioFocusChange, AudioManager.STREAM_VOICE_CALL,
                                                              AUDIOFOCUS_GAIN_TRANSIENT_MAY_DUCK);
    return requestResult == AudioManager.AUDIOFOCUS_REQUEST_GRANTED;
  }

  @Override
  protected void releaseAudioFocusImpl()
  {
    mAudioManager.abandonAudioFocus(this::onAudioFocusChange);
  }
}
