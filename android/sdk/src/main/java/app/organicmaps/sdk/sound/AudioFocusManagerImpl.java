package app.organicmaps.sdk.sound;

import static android.media.AudioManager.AUDIOFOCUS_GAIN_TRANSIENT_MAY_DUCK;

import android.content.Context;
import android.media.AudioFocusRequest;
import android.media.AudioManager;
import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;

@RequiresApi(26)
final class AudioFocusManagerImpl extends AudioFocusManager
{
  @NonNull
  private final AudioFocusRequest mAudioFocusRequest;

  public AudioFocusManagerImpl(@NonNull Context context, @NonNull OnAudioFocusLost onAudioFocusLost)
  {
    super(context, onAudioFocusLost);
    mAudioFocusRequest = new AudioFocusRequest.Builder(AUDIOFOCUS_GAIN_TRANSIENT_MAY_DUCK)
                             .setAudioAttributes(AUDIO_ATTRIBUTES)
                             .setOnAudioFocusChangeListener(this::onAudioFocusChange)
                             .build();
  }

  @Override
  protected boolean requestAudioFocusImpl()
  {
    final int requestResult = mAudioManager.requestAudioFocus(mAudioFocusRequest);
    return requestResult == AudioManager.AUDIOFOCUS_REQUEST_GRANTED;
  }

  @Override
  protected void releaseAudioFocusImpl()
  {
    mAudioManager.abandonAudioFocusRequest(mAudioFocusRequest);
  }
}
