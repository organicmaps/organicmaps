package app.organicmaps.sdk.sound;

import static android.media.AudioManager.AUDIOFOCUS_GAIN_TRANSIENT_MAY_DUCK;

import android.content.Context;
import android.media.AudioFocusRequest;
import android.media.AudioManager;
import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;
import app.organicmaps.sdk.util.log.Logger;

@RequiresApi(26)
final class AudioFocusManagerImpl extends AudioFocusManager
{
  private static final String TAG = AudioFocusManagerImpl.class.getSimpleName();

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
  public boolean requestAudioFocus()
  {
    final int requestResult = mAudioManager.requestAudioFocus(mAudioFocusRequest);
    mPlaybackAllowed = requestResult == AudioManager.AUDIOFOCUS_REQUEST_GRANTED;
    if (!mPlaybackAllowed)
      Logger.w(TAG, "Audio focus request failed");
    return mPlaybackAllowed;
  }

  @Override
  public void releaseAudioFocus()
  {
    mAudioManager.abandonAudioFocusRequest(mAudioFocusRequest);
    mPlaybackAllowed = false;
  }
}
