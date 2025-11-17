package app.organicmaps.sdk.sound;

import android.content.Context;
import android.media.AudioAttributes;
import android.media.AudioDeviceInfo;
import android.media.AudioManager;
import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;
import app.organicmaps.sdk.util.log.Logger;

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

  private static final String TAG = AudioFocusManager.class.getSimpleName();

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

  public final boolean requestAudioFocus()
  {
    mPlaybackAllowed = requestAudioFocusImpl();
    if (!mPlaybackAllowed)
    {
      Logger.w(TAG, "Audio focus request failed");
      return false;
    }

    if (isBluetoothScoDeviceUsed())
    {
      Logger.d(TAG, "Bluetooth SCO device is used for audio output");
      mAudioManager.setMode(AudioManager.MODE_IN_COMMUNICATION);
    }
    else
      mAudioManager.setMode(AudioManager.MODE_NORMAL);
    return mPlaybackAllowed;
  }

  public final void releaseAudioFocus()
  {
    releaseAudioFocusImpl();
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

  protected abstract boolean requestAudioFocusImpl();
  protected abstract void releaseAudioFocusImpl();

  protected void onAudioFocusChange(int focusChange)
  {
    Logger.w(TAG, "Unexpected audio focus change: " + focusChange);
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
