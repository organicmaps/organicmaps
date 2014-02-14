package com.mapswithme.util;

import java.util.ArrayList;

import android.content.Context;
import android.content.Intent;
import android.speech.RecognizerIntent;

public class InputUtils
{
  private static Boolean mVoiceInputSupported = null;

  public static boolean isVoiceInputSupported(Context context)
  {
    if (mVoiceInputSupported == null)
    {
      mVoiceInputSupported = Utils.isIntentSupported(context, new Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH));
    }
    return mVoiceInputSupported;
  }

  public static Intent createIntentForVoiceRecognition(String promptText)
  {
    final Intent vrIntent = new Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH);
    vrIntent.putExtra(RecognizerIntent.EXTRA_LANGUAGE_MODEL, RecognizerIntent.LANGUAGE_MODEL_WEB_SEARCH)
            .putExtra(RecognizerIntent.EXTRA_PROMPT, promptText);

    return vrIntent;
  }

  /**
   * @param vrIntentResult
   * @return most confident result or null if nothing is available
   */
  public static String getMostConfidentResult(Intent vrIntentResult)
  {
    final ArrayList<String> recongnizedStrings
      = vrIntentResult.getStringArrayListExtra(RecognizerIntent.EXTRA_RESULTS);

    if (recongnizedStrings == null)
      return null;

    return recongnizedStrings.isEmpty() ? null : recongnizedStrings.get(0);
  }

  private InputUtils() { /* static class */ }
}
