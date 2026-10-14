package app.organicmaps.editor;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.content.Context;
import android.content.ContextWrapper;
import android.content.res.Configuration;
import android.content.res.Resources;
import androidx.annotation.NonNull;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import app.organicmaps.MwmApplication;
import app.organicmaps.sdk.editor.Editor;
import app.organicmaps.sdk.util.Language;
import app.organicmaps.sdk.util.Utils;
import java.util.Arrays;
import java.util.Locale;
import java.util.concurrent.FutureTask;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class FeatureCategorySearchTest
{
  private static final String BOAT_RENTAL = "amenity-boat_rental";

  private static class ResourceCountingContext extends ContextWrapper
  {
    int mResourceReads;

    ResourceCountingContext(Context context)
    {
      super(context);
    }

    @NonNull
    @Override
    public Resources getResources()
    {
      ++mResourceReads;
      return super.getResources();
    }
  }

  private ResourceCountingContext contextFor(String tag)
  {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
    Configuration configuration = new Configuration(context.getResources().getConfiguration());
    configuration.setLocale(Locale.forLanguageTag(tag));
    return new ResourceCountingContext(context.createConfigurationContext(configuration));
  }

  private void assertFinds(Context context, String locale, String query, String type)
  {
    assertTrue(query, Arrays.asList(Editor.nativeSearchCreatableFeatureTypes(context, query, locale)).contains(type));
  }

  @Test
  public void indexesPickerNamesAndRefreshesBothEntrypoints() throws Exception
  {
    FutureTask<Void> task = new FutureTask<>(() -> {
      Context target = InstrumentationRegistry.getInstrumentation().getTargetContext();
      MwmApplication.from(target).initOrganicMaps(null);

      ResourceCountingContext german = contextFor("de");
      String germanLocale = Language.getResourceLocale(german);
      german.mResourceReads = 0;
      String[] types = Editor.nativeGetAllCreatableFeatureTypes(german, germanLocale);
      assertTrue(types.length > 0);
      assertTrue(german.mResourceReads >= types.length);

      String germanName = Utils.getLocalizedFeatureType(german, BOAT_RENTAL);
      assertEquals("Bootsverleih", germanName);
      german.mResourceReads = 0;
      assertFinds(german, germanLocale, germanName, BOAT_RENTAL);
      assertFinds(german, germanLocale, "Boot mieten", BOAT_RENTAL);
      assertFinds(german, germanLocale, "Boat Rental", BOAT_RENTAL);
      assertArrayEquals(types, Editor.nativeGetAllCreatableFeatureTypes(german, germanLocale));
      assertEquals(0, german.mResourceReads);

      for (String type : types)
        assertFinds(german, germanLocale, Utils.getLocalizedFeatureType(german, type), type);

      ResourceCountingContext french = contextFor("fr");
      String frenchLocale = Language.getResourceLocale(french);
      String frenchName = Utils.getLocalizedFeatureType(french, BOAT_RENTAL);
      assertEquals("Location de bateaux", frenchName);
      french.mResourceReads = 0;
      assertFinds(french, frenchLocale, frenchName, BOAT_RENTAL);
      assertTrue(french.mResourceReads >= types.length);

      french.mResourceReads = 0;
      assertFinds(french, frenchLocale, "Boat Rental", BOAT_RENTAL);
      assertFalse(Arrays.asList(Editor.nativeSearchCreatableFeatureTypes(french, germanName, frenchLocale))
                      .contains(BOAT_RENTAL));
      assertFalse(Arrays.asList(Editor.nativeSearchCreatableFeatureTypes(french, "Boot mieten", frenchLocale))
                      .contains(BOAT_RENTAL));
      assertArrayEquals(types, Editor.nativeGetAllCreatableFeatureTypes(french, frenchLocale));
      assertEquals(0, french.mResourceReads);

      german.mResourceReads = 0;
      assertArrayEquals(types, Editor.nativeGetAllCreatableFeatureTypes(german, germanLocale));
      assertTrue(german.mResourceReads >= types.length);
      assertFinds(german, germanLocale, germanName, BOAT_RENTAL);
      return null;
    });
    InstrumentationRegistry.getInstrumentation().runOnMainSync(task);
    task.get();
  }
}
