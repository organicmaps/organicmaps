package app.organicmaps.car.screens.download;

import androidx.annotation.NonNull;
import androidx.car.app.model.Action;
import androidx.car.app.model.CarIcon;
import androidx.car.app.model.Header;
import androidx.car.app.model.MessageTemplate;
import androidx.car.app.model.Template;
import androidx.core.graphics.drawable.IconCompat;
import app.organicmaps.R;
import app.organicmaps.car.screens.base.BaseScreen;
import app.organicmaps.car.util.Colors;
import app.organicmaps.sdk.downloader.CountryItem;
import app.organicmaps.sdk.util.StringUtils;
import java.util.List;

public abstract class DownloadMapsScreen extends BaseScreen
{
  public static final String MARKER = "Downloader";

  private boolean mIsCancelActionDisabled = false;

  @NonNull
  private final List<CountryItem> mMissingMaps;

  DownloadMapsScreen(@NonNull final DownloadMapsScreenBuilder builder)
  {
    super(builder.mCarContext);
    setMarker(MARKER);
    setResult(false);

    mMissingMaps = DownloaderHelpers.getCountryItemsFromIds(builder.mMissingMaps);
  }

  @NonNull
  @Override
  public final Template onGetTemplate()
  {
    final MessageTemplate.Builder builder = new MessageTemplate.Builder(getText(getMapsSize(mMissingMaps)));
    final Header.Builder headerBuilder = new Header.Builder();
    headerBuilder.setStartHeaderAction(getHeaderAction());
    headerBuilder.setTitle(getTitle());

    builder.setHeader(headerBuilder.build());
    builder.setIcon(getIcon());
    builder.addAction(getDownloadAction());
    if (!mIsCancelActionDisabled)
      builder.addAction(getCancelAction());

    return builder.build();
  }

  @NonNull
  protected abstract String getTitle();

  @NonNull
  protected abstract String getText(@NonNull final String mapsSize);

  @NonNull
  protected abstract Action getHeaderAction();

  protected void disableCancelAction()
  {
    mIsCancelActionDisabled = true;
  }

  @NonNull
  protected List<CountryItem> getMissingMaps()
  {
    return mMissingMaps;
  }

  @NonNull
  private CarIcon getIcon()
  {
    return new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), R.drawable.ic_download)).build();
  }

  @NonNull
  private Action getDownloadAction()
  {
    return new Action.Builder()
        .setFlags(Action.FLAG_DEFAULT)
        .setTitle(getCarContext().getString(R.string.download))
        .setBackgroundColor(Colors.BUTTON_ACCEPT)
        .setOnClickListener(this::onDownload)
        .build();
  }

  @NonNull
  private Action getCancelAction()
  {
    return new Action.Builder()
        .setTitle(getCarContext().getString(R.string.cancel))
        .setOnClickListener(this::finish)
        .build();
  }

  private void onDownload()
  {
    getScreenManager().pushForResult(new DownloaderScreen(getCarContext(), mMissingMaps, mIsCancelActionDisabled),
                                     result -> {
                                       setResult(result);
                                       finish();
                                     });
  }

  @NonNull
  private String getMapsSize(@NonNull final List<CountryItem> countries)
  {
    return StringUtils.getFileSizeString(getCarContext(), DownloaderHelpers.getMapsSize(countries));
  }
}
