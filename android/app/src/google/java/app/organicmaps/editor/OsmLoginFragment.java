package app.organicmaps.editor;

import android.net.Uri;
import android.view.View;
import androidx.annotation.NonNull;
import app.organicmaps.R;
import app.organicmaps.sdk.editor.OsmOAuth;
import app.organicmaps.util.Utils;

public class OsmLoginFragment extends BaseOsmLoginFragment
{
  @Override
  protected void setupLogin(@NonNull View view)
  {
    view.findViewById(R.id.login)
        .setOnClickListener(v
                            -> Utils.openUri(requireContext(), Uri.parse(OsmOAuth.nativeGetOAuth2Url()),
                                             R.string.browser_not_available));
  }
}
