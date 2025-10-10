package app.organicmaps.sdk.widget.roadshield;

import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.style.CharacterStyle;
import android.text.style.ImageSpan;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.routing.roadshield.RoadShieldInfo;

public class RoadShieldUtils
{
  private static final String NON_BREAKING_SPACE = "\u00A0";

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

    street = prepareText(street, roadShieldInfo);
    final SpannableStringBuilder text = new SpannableStringBuilder(street);
    int nextRoadShieldStartIndex = 0;

    if (roadShieldInfo.junctionShields() != null)
    {
      final RoadShieldDrawable shield =
          new RoadShieldDrawable(roadShieldInfo.junctionShields()[0], textSize, drawOutline);
      final CharacterStyle shieldSpan = roadShieldSpanCreator.create(shield);
      final int start = street.indexOf('[');
      final int end = street.indexOf(']', start) + 1;
      text.setSpan(shieldSpan, start, end, Spannable.SPAN_INCLUSIVE_INCLUSIVE);

      if (roadShieldInfo.junctionShields().length > 1)
      {
        final RoadShieldDrawable secondShield =
            new RoadShieldDrawable(roadShieldInfo.junctionShields()[1], textSize, drawOutline);
        final CharacterStyle secondShieldSpan = roadShieldSpanCreator.create(secondShield);
        text.setSpan(secondShieldSpan, end + 1, end + 2, Spannable.SPAN_INCLUSIVE_INCLUSIVE);
      }

      nextRoadShieldStartIndex = end;
    }

    if (roadShieldInfo.targetRoadShields() != null)
    {
      final RoadShieldDrawable shield =
          new RoadShieldDrawable(roadShieldInfo.targetRoadShields()[0], textSize, drawOutline);
      final CharacterStyle shieldSpan = roadShieldSpanCreator.create(shield);
      final int start = street.indexOf('[', nextRoadShieldStartIndex);
      final int end = street.indexOf(']', start) + 1;
      text.setSpan(shieldSpan, start, end, Spannable.SPAN_INCLUSIVE_INCLUSIVE);

      if (roadShieldInfo.targetRoadShields().length > 1)
      {
        final RoadShieldDrawable secondShield =
            new RoadShieldDrawable(roadShieldInfo.targetRoadShields()[1], textSize, drawOutline);
        final CharacterStyle secondShieldSpan = roadShieldSpanCreator.create(secondShield);
        text.setSpan(secondShieldSpan, end + 1, end + 2, Spannable.SPAN_INCLUSIVE_INCLUSIVE);
      }
    }

    return text;
  }

  @NonNull
  private static String prepareText(@NonNull String text, @NonNull RoadShieldInfo roadShieldInfo)
  {
    final StringBuilder sb = new StringBuilder(text);

    int nextRoadShieldStartIndex = 0;

    if (roadShieldInfo.junctionShields() != null && roadShieldInfo.junctionShields().length > 1)
    {
      final int start = sb.indexOf("[");
      final int end = sb.indexOf("]", start) + 1;
      sb.insert(end, NON_BREAKING_SPACE + NON_BREAKING_SPACE + NON_BREAKING_SPACE);
      nextRoadShieldStartIndex = end;
    }

    if (roadShieldInfo.targetRoadShields() != null && roadShieldInfo.targetRoadShields().length > 1)
    {
      final int start = sb.indexOf("[", nextRoadShieldStartIndex);
      final int end = sb.indexOf("]", start) + 1;
      sb.insert(end, NON_BREAKING_SPACE + NON_BREAKING_SPACE);
    }

    return sb.toString();
  }

  @NonNull
  private static CharacterStyle createRoadShieldSpan(@NonNull RoadShieldDrawable roadShieldDrawable)
  {
    return new ImageSpan(roadShieldDrawable, ImageSpan.ALIGN_BOTTOM);
  }
}
