package com.mapswithme.util;

import android.content.Context;
import android.content.Intent;
import android.speech.RecognizerIntent;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;

import java.util.ArrayList;

public class InputUtils
{
  private static Boolean mVoiceInputSupported = null;

  private InputUtils() { /* static class */ }

  public static boolean isVoiceInputSupported(Context context)
  {
    if (mVoiceInputSupported == null)
      mVoiceInputSupported = Utils.isIntentSupported(context, new Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH));

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
    final ArrayList<String> recognizedStrings
        = vrIntentResult.getStringArrayListExtra(RecognizerIntent.EXTRA_RESULTS);

    if (recognizedStrings == null)
      return null;

    return recognizedStrings.isEmpty() ? null : recognizedStrings.get(0);
  }

  public static void hideKeyboard(View view)
  {
    final Context c = view.getContext();
    final InputMethodManager imm = (InputMethodManager) c.getSystemService(Context.INPUT_METHOD_SERVICE);
    imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
  }

  /*
   Hacky method to remove focus from the only EditText at activity
   */
  public static void removeFocusEditTextHack(EditText editText)
  {
    editText.setFocusableInTouchMode(false);
    editText.setFocusable(false);
    editText.setFocusableInTouchMode(true);
    editText.setFocusable(true);
  }
}
