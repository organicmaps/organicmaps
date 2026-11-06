package app.organicmaps.test;

import static org.junit.Assert.assertEquals;

import android.content.Context;
import android.content.res.Configuration;
import android.os.Build;
import androidx.test.platform.app.InstrumentationRegistry;
import app.organicmaps.sdk.util.Language;
import java.util.Locale;
import org.junit.Test;

public class ResourceLocaleTest
{
  private Context contextFor(String tag)
  {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
    Configuration configuration = new Configuration(context.getResources().getConfiguration());
    configuration.setLocale(Locale.forLanguageTag(tag));
    return context.createConfigurationContext(configuration);
  }

  @Test
  public void resourceLocaleUsesThePickersContext()
  {
    String systemLocale = Language.getDefaultLocale();
    assertEquals("cy-GB", Language.getResourceLocale(contextFor("cy-GB")));
    assertEquals("pt-BR", Language.getResourceLocale(contextFor("pt-BR")));
    assertEquals(systemLocale, Language.getDefaultLocale());
  }

  @Test
  public void resourceLocaleKeepsChineseScript()
  {
    assertEquals("zh-Hant-TW", Language.getResourceLocale(contextFor("zh-Hant-TW")));
    assertEquals("zh-Hans-CN", Language.getResourceLocale(contextFor("zh-Hans-CN")));
  }

  @Test
  public void resourceLocaleDropsRegionalPreferences()
  {
    String locale = "en-GB-u-fw-mon";
    String expected = Build.VERSION.SDK_INT >= Build.VERSION_CODES.O ? "en-GB" : locale;
    assertEquals(expected, Language.getResourceLocale(contextFor(locale)));
  }
}
