package app.organicmaps.util;

import android.app.Activity;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.core.text.TextUtilsCompat;
import androidx.core.view.ViewCompat;

import java.util.Arrays;
import java.util.List;
import java.util.Locale;

public class RtlUtils
{
    private final static List<String> rtlLocalesWithTranslation = Arrays.asList("ar", "fa");

    public static void manageRtl(@NonNull final Activity activity)
    {
      final String currentLanguage = Locale.getDefault().getLanguage();
      final boolean isRTL = TextUtilsCompat.getLayoutDirectionFromLocale(Locale.getDefault()) == ViewCompat.LAYOUT_DIRECTION_RTL;
      if (isRTL && rtlLocalesWithTranslation.contains(currentLanguage))
        activity.getWindow().getDecorView().setLayoutDirection(View.LAYOUT_DIRECTION_RTL);
      else
        activity.getWindow().getDecorView().setLayoutDirection(View.LAYOUT_DIRECTION_LTR);
    }
}
