package app.organicmaps.editor;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.content.Context;
import android.content.res.Configuration;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import app.organicmaps.MwmApplication;
import app.organicmaps.sdk.editor.Editor;
import app.organicmaps.sdk.util.Language;
import app.organicmaps.sdk.util.Utils;
import java.util.Arrays;
import java.util.Locale;
import java.util.concurrent.Callable;
import java.util.concurrent.FutureTask;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class FeatureCategorySearchTest
{
  private static final String BOAT_RENTAL = "amenity-boat_rental";

  private Context contextFor(String tag)
  {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
    Configuration configuration = new Configuration(context.getResources().getConfiguration());
    configuration.setLocale(Locale.forLanguageTag(tag));
    return context.createConfigurationContext(configuration);
  }

  private void assertFinds(Context context, String locale, String query, String type)
  {
    assertTrue(query, Arrays.asList(Editor.nativeSearchCreatableFeatureTypes(context, query, locale)).contains(type));
  }

  private void runOnMainThread(Callable<Void> action) throws Exception
  {
    FutureTask<Void> task = new FutureTask<>(action);
    InstrumentationRegistry.getInstrumentation().runOnMainSync(task);
    task.get();
  }

  @Before
  public void initializeCore() throws Exception
  {
    runOnMainThread(() -> {
      Context target = InstrumentationRegistry.getInstrumentation().getTargetContext();
      MwmApplication.from(target).initOrganicMaps(null);
      return null;
    });
  }

  @Test
  public void findsDisplayedNamesAndKeywords() throws Exception
  {
    runOnMainThread(() -> {
      Context german = contextFor("de");
      String germanLocale = Language.getResourceLocale(german);
      String[] types = Editor.nativeGetAllCreatableFeatureTypes(german, germanLocale);
      assertTrue(types.length > 0);

      String germanName = Utils.getLocalizedFeatureType(german, BOAT_RENTAL);
      assertEquals("Bootsverleih", germanName);
      assertFinds(german, germanLocale, germanName, BOAT_RENTAL);
      assertFinds(german, germanLocale, "Boot mieten", BOAT_RENTAL);
      assertFinds(german, germanLocale, "Boat Rental", BOAT_RENTAL);

      for (String type : types)
        assertFinds(german, germanLocale, Utils.getLocalizedFeatureType(german, type), type);
      return null;
    });
  }

  @Test
  public void localeChangesReplaceNamesAndKeywordLanguages() throws Exception
  {
    runOnMainThread(() -> {
      Context german = contextFor("de");
      String germanLocale = Language.getResourceLocale(german);
      Editor.nativeGetAllCreatableFeatureTypes(german, germanLocale);
      String germanName = Utils.getLocalizedFeatureType(german, BOAT_RENTAL);
      assertFinds(german, germanLocale, germanName, BOAT_RENTAL);

      Context french = contextFor("fr");
      String frenchLocale = Language.getResourceLocale(french);
      String frenchName = Utils.getLocalizedFeatureType(french, BOAT_RENTAL);
      assertEquals("Location de bateaux", frenchName);
      assertFinds(french, frenchLocale, frenchName, BOAT_RENTAL);

      assertFinds(french, frenchLocale, "Boat Rental", BOAT_RENTAL);
      assertFalse(Arrays.asList(Editor.nativeSearchCreatableFeatureTypes(french, germanName, frenchLocale))
                      .contains(BOAT_RENTAL));
      assertFalse(Arrays.asList(Editor.nativeSearchCreatableFeatureTypes(french, "Boot mieten", frenchLocale))
                      .contains(BOAT_RENTAL));

      Editor.nativeGetAllCreatableFeatureTypes(german, germanLocale);
      assertFinds(german, germanLocale, germanName, BOAT_RENTAL);
      assertFinds(german, germanLocale, "Boot mieten", BOAT_RENTAL);
      assertFalse(Arrays.asList(Editor.nativeSearchCreatableFeatureTypes(german, frenchName, germanLocale))
                      .contains(BOAT_RENTAL));
      return null;
    });
  }

  @Test
  public void creatableTypesStaySortedAcrossLocales() throws Exception
  {
    runOnMainThread(() -> {
      Context german = contextFor("de");
      String germanLocale = Language.getResourceLocale(german);
      String[] types = Editor.nativeGetAllCreatableFeatureTypes(german, germanLocale);
      assertTrue(types.length > 0);
      String[] sorted = types.clone();
      Arrays.sort(sorted);
      assertArrayEquals(sorted, types);
      assertArrayEquals(types, Editor.nativeGetAllCreatableFeatureTypes(german, germanLocale));

      Context french = contextFor("fr");
      assertArrayEquals(types, Editor.nativeGetAllCreatableFeatureTypes(french, Language.getResourceLocale(french)));
      assertArrayEquals(types, Editor.nativeGetAllCreatableFeatureTypes(german, germanLocale));
      return null;
    });
  }
}
