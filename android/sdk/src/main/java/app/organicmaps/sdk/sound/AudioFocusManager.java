package app.organicmaps.sdk.sound;

import android.content.Context;
import android.media.AudioAttributes;
import android.media.AudioDeviceInfo;
import android.media.AudioManager;
import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;
import androidx.media.AudioAttributesCompat;
import androidx.media.AudioFocusRequestCompat;
import androidx.media.AudioManagerCompat;
import app.organicmaps.sdk.util.log.Logger;

final class AudioFocusManager
{
  public interface OnAudioFocusLost
  {
    void onAudioFocusLost();
  }

  // Keep in sync with audioAttributesCompat in the constructor below.
  public static final AudioAttributes AUDIO_ATTRIBUTES =
      new AudioAttributes.Builder()
          .setUsage(AudioAttributes.USAGE_ASSISTANCE_NAVIGATION_GUIDANCE)
          .setContentType(AudioAttributes.CONTENT_TYPE_SPEECH)
          .build();

  private static final String TAG = AudioFocusManager.class.getSimpleName();

  @NonNull
  private final AudioManager mAudioManager;
  @NonNull
  private final OnAudioFocusLost mOnAudioFocusLost;
  @NonNull
  private final AudioFocusRequestCompat mAudioFocusRequest;
  // The request is granted and not abandoned yet. Tells a redundant releaseAudioFocus() apart from
  // a real one, so that cleanup paths do not reset the system-wide audio mode out of the blue.
  private boolean mFocusHeld = false;
  private boolean mPlaybackAllowed = false;

  public AudioFocusManager(@NonNull Context context, @NonNull OnAudioFocusLost onAudioFocusLost)
  {
    mAudioManager = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
    mOnAudioFocusLost = onAudioFocusLost;
    // Keep in sync with AUDIO_ATTRIBUTES above.
    final AudioAttributesCompat audioAttributesCompat =
        new AudioAttributesCompat.Builder()
            .setUsage(AudioAttributesCompat.USAGE_ASSISTANCE_NAVIGATION_GUIDANCE)
            .setContentType(AudioAttributesCompat.CONTENT_TYPE_SPEECH)
            .build();
    mAudioFocusRequest = new AudioFocusRequestCompat.Builder(AudioManagerCompat.AUDIOFOCUS_GAIN_TRANSIENT_MAY_DUCK)
                             .setAudioAttributes(audioAttributesCompat)
                             .setOnAudioFocusChangeListener(this::onAudioFocusChange)
                             .build();
  }

  public boolean requestAudioFocus()
  {
    final int result = AudioManagerCompat.requestAudioFocus(mAudioManager, mAudioFocusRequest);
    mPlaybackAllowed = result == AudioManager.AUDIOFOCUS_REQUEST_GRANTED;
    if (!mPlaybackAllowed)
    {
      Logger.w(TAG, "Audio focus request failed");
      return false;
    }

    mFocusHeld = true;
    if (isBluetoothScoDeviceUsed())
    {
      Logger.d(TAG, "Bluetooth SCO device is used for audio output");
      mAudioManager.setMode(AudioManager.MODE_IN_COMMUNICATION);
    }
    else
      mAudioManager.setMode(AudioManager.MODE_NORMAL);
    return mPlaybackAllowed;
  }

  public void releaseAudioFocus()
  {
    if (!mFocusHeld)
      return;

    mFocusHeld = false;
    AudioManagerCompat.abandonAudioFocusRequest(mAudioManager, mAudioFocusRequest);
    mPlaybackAllowed = false;
    mAudioManager.setMode(AudioManager.MODE_NORMAL);
  }

  public boolean isMusicActive()
  {
    return mAudioManager.isMusicActive();
  }

  public boolean isBluetoothScoDeviceUsed()
  {
    if (!mAudioManager.isBluetoothScoAvailableOffCall())
      return false;

    return android.os.Build.VERSION.SDK_INT >= 31 ? ImplApi31.isBluetoothScoOn(mAudioManager)
                                                  : mAudioManager.isBluetoothScoOn();
  }

  private void onAudioFocusChange(int focusChange)
  {
    Logger.i(TAG, "Audio focus change: " + focusChange);
    if (focusChange == AudioManager.AUDIOFOCUS_GAIN || focusChange == AudioManager.AUDIOFOCUS_GAIN_TRANSIENT
        || focusChange == AudioManager.AUDIOFOCUS_GAIN_TRANSIENT_MAY_DUCK)
    {
      mPlaybackAllowed = true;
    }
    else if (focusChange == AudioManager.AUDIOFOCUS_LOSS || focusChange == AudioManager.AUDIOFOCUS_LOSS_TRANSIENT)
    {
      mPlaybackAllowed = false;
      mOnAudioFocusLost.onAudioFocusLost();
    }
    // Continue playing (potentially important) voice command on AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK.
  }

  @RequiresApi(31)
  private static class ImplApi31
  {
    private static boolean isBluetoothScoOn(@NonNull AudioManager audioManager)
    {
      final AudioDeviceInfo device = audioManager.getCommunicationDevice();
      return device != null && device.getType() == AudioDeviceInfo.TYPE_BLUETOOTH_SCO;
    }
  }
}
