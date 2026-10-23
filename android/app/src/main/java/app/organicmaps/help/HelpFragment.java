package app.organicmaps.help;

import android.os.Bundle;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.method.LinkMovementMethod;
import android.text.style.ClickableSpan;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import androidx.annotation.IdRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.view.ViewCompat;
import app.organicmaps.BuildConfig;
import app.organicmaps.R;
import app.organicmaps.base.BaseMwmFragment;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.util.DateUtils;
import app.organicmaps.util.Graphics;
import app.organicmaps.util.Utils;
import app.organicmaps.util.WindowInsetUtils.ScrollableContentInsetsListener;
import app.organicmaps.widget.DonationView;

public class HelpFragment extends BaseMwmFragment implements View.OnClickListener
{
  private static final String ORGANIC_MAPS_URL = "https://organicmaps.app";
  private static final String OSM_COPYRIGHT_URL = "https://www.openstreetmap.org/copyright";
  private static final String SOURCE_CODE_URL = "https://github.com/Ramzess-II/NoGPSMaps";
  // The channels, documents and support of Organic Maps, not of NonGPS Maps.
  private static final int[] ORGANIC_MAPS_ITEMS = {R.id.news,
                                                   R.id.web,
                                                   R.id.email,
                                                   R.id.telegram,
                                                   R.id.instagram,
                                                   R.id.facebook,
                                                   R.id.twitter,
                                                   R.id.matrix,
                                                   R.id.mastodon,
                                                   R.id.faq,
                                                   R.id.report,
                                                   R.id.support_us,
                                                   R.id.term_of_use_link,
                                                   R.id.privacy_policy};

  private String mDonateUrl;

  private TextView setupItem(@IdRes int id, boolean tint, @NonNull View frame)
  {
    final TextView view = frame.findViewById(id);
    view.setOnClickListener(this);
    if (tint)
      Graphics.tint(view);
    return view;
  }

  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    mDonateUrl = Utils.getDonateUrl(requireContext());
    View root = inflater.inflate(R.layout.about, container, false);

    ((TextView) root.findViewById(R.id.version)).setText(BuildConfig.VERSION_NAME);

    final String dataVersion = DateUtils.getShortDateFormatter().format(Framework.getDataVersion());
    final TextView osmPresentationView = root.findViewById(R.id.osm_presentation);
    if (osmPresentationView != null)
      osmPresentationView.setText(getString(R.string.osm_presentation, dataVersion));

    // The project and the map data are credited with clickable names, as Organic Maps and OpenStreetMap ask.
    linkNames(root.findViewById(R.id.based_on), "Organic Maps", ORGANIC_MAPS_URL, null, null);
    linkNames(root.findViewById(R.id.map_data), "OpenStreetMap", OSM_COPYRIGHT_URL, "Organic Maps", ORGANIC_MAPS_URL);
    for (int id : ORGANIC_MAPS_ITEMS)
      root.findViewById(id).setVisibility(View.GONE);
    setupItem(R.id.github, true, root).setText(R.string.nogps_source_code);
    setupItem(R.id.openstreetmap, true, root);

    DonationView donationView = root.findViewById(R.id.donate);
    if (TextUtils.isEmpty(mDonateUrl))
    {
      donationView.setVisibility(View.GONE);
      donationView.setOnDonateClickListener(null);
    }
    else
    {
      donationView.setVisibility(View.VISIBLE);
      donationView.setOnDonateClickListener(() -> {
        Utils.openUrl(requireActivity(), mDonateUrl);
        Framework.nativeDidShowDonationPage();
      });
    }

    if (BuildConfig.REVIEW_URL.isEmpty())
      root.findViewById(R.id.rate).setVisibility(View.GONE);
    else
      setupItem(R.id.rate, true, root);

    setupItem(R.id.copyright, false, root);

    ViewCompat.setOnApplyWindowInsetsListener(root, new ScrollableContentInsetsListener(root));

    return root;
  }

  /**
   * Makes the names in the text clickable links, the second name is optional.
   */
  private void linkNames(@NonNull TextView view, @NonNull String name, @NonNull String url, @Nullable String secondName,
                         @Nullable String secondUrl)
  {
    final SpannableString text = new SpannableString(view.getText());
    linkName(text, name, url);
    if (secondName != null && secondUrl != null)
      linkName(text, secondName, secondUrl);
    view.setText(text);
    view.setMovementMethod(LinkMovementMethod.getInstance());
  }

  private void linkName(@NonNull SpannableString text, @NonNull String name, @NonNull String url)
  {
    final int start = text.toString().indexOf(name);
    if (start < 0)
      return;
    text.setSpan(new ClickableSpan() {
      @Override
      public void onClick(@NonNull View widget)
      {
        Utils.openUrl(requireActivity(), url);
      }
    }, start, start + name.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
  }

  @Override
  public void onClick(View v)
  {
    final int id = v.getId();
    if (id == R.id.github)
      Utils.openUrl(requireActivity(), SOURCE_CODE_URL);
    else if (id == R.id.openstreetmap)
      Utils.openUrl(requireActivity(), getString(R.string.osm_wiki_about_url));
    else if (id == R.id.rate)
      Utils.openAppInMarket(requireActivity(), BuildConfig.REVIEW_URL);
    else if (id == R.id.copyright)
      ((HelpActivity) requireActivity()).stackFragment(CopyrightFragment.class, getString(R.string.copyright), null);
  }
}
