package com.mapswithme.maps.sound;

import android.content.Context;
import android.media.AudioFocusRequest;
import android.media.AudioManager;
import android.os.Build;

import androidx.annotation.Nullable;

import static android.media.AudioManager.AUDIOFOCUS_GAIN_TRANSIENT_EXCLUSIVE;

public class AudioFocusManager
{
  @Nullable
  private AudioManager audioManager = null;

  public AudioFocusManager(@Nullable Context context)
  {
    if (context != null)
      audioManager = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
  }

  public void requestAudioFocus()
  {
    if (audioManager != null)
    {
      if (Build.VERSION.SDK_INT >= 26)
        audioManager.requestAudioFocus(new AudioFocusRequest.Builder(AUDIOFOCUS_GAIN_TRANSIENT_EXCLUSIVE).build());
      else
        audioManager.requestAudioFocus(focusChange -> {}, AudioManager.STREAM_VOICE_CALL, AUDIOFOCUS_GAIN_TRANSIENT_EXCLUSIVE);
    }
  }

  public void releaseAudioFocus()
  {
    if (audioManager != null)
    {
      if (Build.VERSION.SDK_INT >= 26 )
        audioManager.abandonAudioFocusRequest(new AudioFocusRequest.Builder(AUDIOFOCUS_GAIN_TRANSIENT_EXCLUSIVE).build());
      else
        audioManager.abandonAudioFocus(focusChange -> {});
    }
  }
}
