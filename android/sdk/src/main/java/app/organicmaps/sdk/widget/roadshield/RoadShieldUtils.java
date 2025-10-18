package app.organicmaps.sdk.widget.roadshield;

import android.text.Spannable;
import android.text.SpannableString;
import android.text.style.CharacterStyle;
import android.text.style.ImageSpan;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.routing.roadshield.RoadShield;
import app.organicmaps.sdk.routing.roadshield.RoadShieldInfo;
import java.util.Objects;

public class RoadShieldUtils
{
  public interface RoadShieldSpanCreator
  {
    @NonNull
    CharacterStyle create(@NonNull RoadShieldDrawable roadShieldDrawable);
  }

  @NonNull
  public static CharSequence createStreetTextWithShields(@NonNull String street,
                                                         @Nullable RoadShieldInfo roadShieldInfo, float textSize)
  {
    return createStreetTextWithShields(RoadShieldUtils::createRoadShieldSpan, street, roadShieldInfo, textSize, true);
  }

  @NonNull
  public static CharSequence createStreetTextWithShields(@NonNull RoadShieldSpanCreator roadShieldSpanCreator,
                                                         @NonNull String street,
                                                         @Nullable RoadShieldInfo roadShieldInfo, float textSize,
                                                         boolean drawOutline)
  {
    if (roadShieldInfo == null)
      return street;

    final SpannableString text = new SpannableString(street);
    if (roadShieldInfo.hasTargetRoadShields())
      applyRoadShieldsSpannables(roadShieldSpanCreator, text, Objects.requireNonNull(roadShieldInfo.targetRoadShields),
                                 roadShieldInfo.targetRoadShieldsIndexStart, roadShieldInfo.targetRoadShieldsIndexEnd,
                                 textSize, drawOutline);
    return text;
  }

  private static void applyRoadShieldsSpannables(@NonNull RoadShieldSpanCreator roadShieldSpanCreator,
                                                 @NonNull SpannableString text, @NonNull RoadShield[] shields,
                                                 int indexStart, int indexEnd, float textSize, boolean drawOutline)
  {
    int offset = 0;
    for (int i = 0; i < shields.length; i++)
    {
      final RoadShieldDrawable shield = new RoadShieldDrawable(shields[i], textSize, drawOutline);
      final CharacterStyle shieldSpan = roadShieldSpanCreator.create(shield);
      final int start = indexStart + offset;
      final int end = i < shields.length - 1 ? start + 1 : indexEnd;
      text.setSpan(shieldSpan, start, end, Spannable.SPAN_EXCLUSIVE_INCLUSIVE);
      offset += 1;
    }
  }

  @NonNull
  private static CharacterStyle createRoadShieldSpan(@NonNull RoadShieldDrawable roadShieldDrawable)
  {
    return new ImageSpan(roadShieldDrawable, ImageSpan.ALIGN_BOTTOM);
  }
}
