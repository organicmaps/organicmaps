package app.organicmaps.widget.placepage.sections;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.app.Activity;
import android.app.Application;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.res.Resources;
import android.graphics.Color;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewStub;
import android.webkit.WebView;
import android.widget.TextView;
import androidx.core.view.accessibility.AccessibilityNodeInfoCompat;
import app.organicmaps.R;
import app.organicmaps.sdk.util.NetworkPolicy;
import app.organicmaps.sdk.util.StringUtils;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockedStatic;
import org.mockito.Mockito;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.Shadows;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.GraphicsMode;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;

@RunWith(RobolectricTestRunner.class)
@GraphicsMode(GraphicsMode.Mode.NATIVE)
@Config(sdk = 35, application = Application.class, shadows = ExpandableNotesAccessibilityTest.HtmlDetection.class)
public class ExpandableNotesAccessibilityTest
{
  private ExpandableNotesView mNotes;
  private TextView mText;
  private TextView mMore;
  private MockedStatic<LayoutInflater> mInflation;
  private MockedStatic<NetworkPolicy> mNetwork;

  @Before
  public void setUp()
  {
    Activity activity = Robolectric.buildActivity(Activity.class).setup().get();
    Resources original = activity.getResources();
    Resources resources =
        new Resources(original.getAssets(), original.getDisplayMetrics(), original.getConfiguration()) {
          @Override
          public int getDimensionPixelSize(int id)
          {
            return 120;
          }
          @Override
          public int getColor(int id, Theme theme)
          {
            return Color.WHITE;
          }
          @Override
          public CharSequence getText(int id)
          {
            if (id == R.string.category_desc_more)
              return "More";
            if (id == R.string.description_less)
              return "Less";
            if (id == R.string.description_expanded)
              return "Expanded";
            if (id == R.string.description_collapsed)
              return "Collapsed";
            return super.getText(id);
          }
        };
    Context context = new ContextWrapper(activity) {
      @Override
      public Resources getResources()
      {
        return resources;
      }
    };
    // Inflate a minimal presentation fixture while testing the real description control.
    LayoutInflater inflater = Mockito.mock(LayoutInflater.class);
    Mockito.when(inflater.inflate(Mockito.eq(R.layout.place_page_notes_widget), Mockito.any(ViewGroup.class)))
        .thenAnswer(call -> {
          ViewGroup parent = call.getArgument(1);
          mText = new TextView(context);
          mText.setId(R.id.tv__notes);
          parent.addView(mText);
          View fade = new View(context);
          fade.setId(R.id.notes_fade_overlay);
          parent.addView(fade);
          mMore = new TextView(context);
          mMore.setId(R.id.notes_more_region);
          mMore.setFocusable(true);
          parent.addView(mMore);
          ViewStub stub = new ViewStub(context);
          stub.setId(R.id.wv__notes_stub);
          stub.setInflatedId(R.id.wv__notes);
          stub.setLayoutResource(R.layout.place_page_notes_webview);
          stub.setLayoutInflater(inflater);
          parent.addView(stub);
          return parent;
        });
    Mockito
        .when(inflater.inflate(Mockito.eq(R.layout.place_page_notes_webview), Mockito.any(ViewGroup.class),
                               Mockito.eq(false)))
        .thenAnswer(call -> new WebView(context));
    mInflation = Mockito.mockStatic(LayoutInflater.class);
    mInflation.when(() -> LayoutInflater.from(context)).thenReturn(inflater);
    mNetwork = Mockito.mockStatic(NetworkPolicy.class);
    mNotes = new ExpandableNotesView(context);
    activity.setContentView(mNotes);
  }

  @After
  public void tearDown()
  {
    if (mInflation != null)
      mInflation.close();
    if (mNetwork != null)
      mNetwork.close();
  }

  private void measure()
  {
    mNotes.measure(View.MeasureSpec.makeMeasureSpec(240, View.MeasureSpec.EXACTLY),
                   View.MeasureSpec.makeMeasureSpec(10000, View.MeasureSpec.AT_MOST));
    mNotes.layout(0, 0, 240, mNotes.getMeasuredHeight());
  }

  @Test
  public void longDescriptionCanExpandAndCollapseWithoutLosingTheButton()
  {
    mNotes.setNotes("A long description with enough words to require several lines. ".repeat(20));
    measure();
    assertEquals(View.VISIBLE, mMore.getVisibility());
    assertTrue(mText.performAccessibilityAction(AccessibilityNodeInfoCompat.ACTION_EXPAND, null));
    assertEquals(Integer.MAX_VALUE, mText.getMaxLines());
    assertEquals(View.VISIBLE, mMore.getVisibility());
    assertEquals("Less", mMore.getText().toString());
    assertEquals("Expanded", mMore.createAccessibilityNodeInfo().getStateDescription());
    assertTrue(mMore.performAccessibilityAction(AccessibilityNodeInfoCompat.ACTION_COLLAPSE, null));
    assertEquals(3, mText.getMaxLines());
    assertEquals("More", mMore.getText().toString());
  }

  @Test
  public void resizingAnExpandedDescriptionKeepsCollapseAvailable()
  {
    mNotes.setNotes("Long description ".repeat(80));
    measure();
    assertTrue(mText.performAccessibilityAction(AccessibilityNodeInfoCompat.ACTION_EXPAND, null));
    mNotes.measure(View.MeasureSpec.makeMeasureSpec(200, View.MeasureSpec.EXACTLY),
                   View.MeasureSpec.makeMeasureSpec(10000, View.MeasureSpec.AT_MOST));
    mNotes.layout(0, 0, 200, mNotes.getMeasuredHeight());
    mNotes.measure(View.MeasureSpec.makeMeasureSpec(200, View.MeasureSpec.EXACTLY),
                   View.MeasureSpec.makeMeasureSpec(10000, View.MeasureSpec.AT_MOST));
    assertEquals(View.VISIBLE, mMore.getVisibility());
    assertTrue(mMore.performAccessibilityAction(AccessibilityNodeInfoCompat.ACTION_COLLAPSE, null));
  }

  @Test
  public void shortAndClearedDescriptionsDoNotExposeExpansion()
  {
    mNotes.setNotes("Short");
    measure();
    assertEquals(View.GONE, mMore.getVisibility());
    assertFalse(mText.performAccessibilityAction(AccessibilityNodeInfoCompat.ACTION_EXPAND, null));
    mNotes.setNotes(null);
    assertEquals(View.GONE, mNotes.getVisibility());
    assertFalse(mNotes.performAccessibilityAction(AccessibilityNodeInfoCompat.ACTION_EXPAND, null));
  }

  @Test
  public void newDescriptionResetsTheExpandedState()
  {
    mNotes.setNotes("Long description ".repeat(80));
    measure();
    assertTrue(mText.performAccessibilityAction(AccessibilityNodeInfoCompat.ACTION_EXPAND, null));
    mNotes.setNotes("Another long description ".repeat(80));
    measure();
    assertEquals(3, mText.getMaxLines());
    assertEquals("More", mMore.getText().toString());
    assertEquals("Collapsed", mMore.createAccessibilityNodeInfo().getStateDescription());
  }

  @Test
  public void htmlOffersBothActionsAndKeepsItsContentFocusable()
  {
    mNotes.setNotes("<table><tr><td>Long HTML description</td></tr></table>");
    measure();
    WebView web = mNotes.findViewById(R.id.wv__notes);
    assertTrue(web.isFocusable());
    var shadow = Shadows.shadowOf(web);
    shadow.getWebViewClient().onPageFinished(web, "about:blank");
    shadow.getLastEvaluatedJavascriptCallback().onReceiveValue("2000");
    assertTrue(web.performAccessibilityAction(AccessibilityNodeInfoCompat.ACTION_EXPAND, null));
    assertEquals(ViewGroup.LayoutParams.WRAP_CONTENT, web.getLayoutParams().height);
    assertTrue(mMore.performAccessibilityAction(AccessibilityNodeInfoCompat.ACTION_COLLAPSE, null));
    assertEquals(120, web.getLayoutParams().height);
  }

  @Implements(StringUtils.class)
  public static class HtmlDetection
  {
    @Implementation
    public static boolean nativeIsHtml(String text)
    {
      return text.startsWith("<");
    }
  }
}
