package app.organicmaps.car.screens.maps;

import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.model.Action;
import androidx.car.app.model.CarColor;
import androidx.car.app.model.CarIcon;
import androidx.car.app.model.Header;
import androidx.car.app.model.MessageTemplate;
import androidx.car.app.model.Template;
import androidx.core.graphics.drawable.IconCompat;
import androidx.lifecycle.LifecycleOwner;
import app.organicmaps.car.R;
import app.organicmaps.sdk.OrganicMaps;
import app.organicmaps.sdk.car.screens.BaseScreen;
import app.organicmaps.sdk.downloader.CountryItem;
import app.organicmaps.sdk.downloader.MapManager;
import app.organicmaps.sdk.util.StringUtils;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class DeleteMapsScreen extends BaseScreen
{
  public static final String MARKER = "DeleteMapsScreen";

  private boolean mIsDeleting = false;
  private int mSubscriptionSlot = 0;

  @NonNull
  private final List<CountryItem> mMaps;
  // Maps still awaiting deletion confirmation from the storage callback.
  @NonNull
  private final Map<String, CountryItem> mPendingMaps = new HashMap<>();

  @NonNull
  private final MapManager.StorageCallback mStorageCallback = new MapManager.StorageCallback() {
    @Override
    public void onStatusChanged(@NonNull List<MapManager.StorageCallbackData> data)
    {
      for (final MapManager.StorageCallbackData item : data)
      {
        final CountryItem map = mPendingMaps.get(item.countryId);
        if (map == null)
          continue;
        map.update();
        if (!map.present)
          mPendingMaps.remove(item.countryId);
      }
      if (mPendingMaps.isEmpty())
      {
        setResult(true);
        finish();
      }
    }

    @Override
    public void onProgress(String countryId, long localSize, long remoteSize)
    {}
  };

  public DeleteMapsScreen(@NonNull CarContext context, @NonNull OrganicMaps organicMapsContext,
                          @NonNull String[] mapIds)
  {
    super(context, organicMapsContext);
    setMarker(MARKER);
    setResult(false);
    mMaps = new ArrayList<>(mapIds.length);
    for (final String id : mapIds)
      mMaps.add(CountryItem.fill(id));
  }

  @Override
  public void onResume(@NonNull LifecycleOwner owner)
  {
    super.onResume(owner);
    // Re-subscribe if the screen resumes while deletion is already in progress.
    if (mIsDeleting && mSubscriptionSlot == 0)
      mSubscriptionSlot = MapManager.nativeSubscribe(mStorageCallback);
  }

  @Override
  public void onPause(@NonNull LifecycleOwner owner)
  {
    super.onPause(owner);
    if (mSubscriptionSlot != 0)
    {
      MapManager.nativeUnsubscribe(mSubscriptionSlot);
      mSubscriptionSlot = 0;
    }
  }

  @NonNull
  @Override
  protected Template onGetTemplateImpl()
  {
    return mIsDeleting ? buildDeletingTemplate() : buildConfirmTemplate();
  }

  private void onDeleteClicked()
  {
    mIsDeleting = true;
    for (final CountryItem map : mMaps)
      mPendingMaps.put(map.id, map);
    mSubscriptionSlot = MapManager.nativeSubscribe(mStorageCallback);
    for (final CountryItem map : mMaps)
      MapManager.nativeDelete(map.id);
    invalidate();
  }

  @NonNull
  private Template buildConfirmTemplate()
  {
    final StringBuilder text = new StringBuilder(getCarContext().getString(R.string.confirmation)).append("\n");
    if (mMaps.size() == 1)
      text.append(mMaps.get(0).name).append(" • ").append(sizeStr(mMaps.get(0).totalSize));
    else
      text.append(getCarContext().getString(R.string.downloader_status_maps))
          .append(" (")
          .append(mMaps.size())
          .append("): ")
          .append(sizeStr(totalSize()));

    final MessageTemplate.Builder builder = new MessageTemplate.Builder(text);
    builder.setIcon(new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), R.drawable.ic_delete)).build());
    builder.setHeader(buildConfirmHeader());
    builder.addAction(buildCancelAction());
    builder.addAction(buildDeleteAction());
    return builder.build();
  }

  @NonNull
  private Template buildDeletingTemplate()
  {
    final MessageTemplate.Builder builder = new MessageTemplate.Builder(getCarContext().getString(R.string.deleting));
    builder.setLoading(true);
    builder.setHeader(new Header.Builder()
                          .setStartHeaderAction(Action.APP_ICON)
                          .setTitle(getCarContext().getString(R.string.delete))
                          .build());
    builder.addAction(buildCancelAction());
    return builder.build();
  }

  @NonNull
  private Header buildConfirmHeader()
  {
    return new Header.Builder()
        .setStartHeaderAction(Action.BACK)
        .setTitle(getCarContext().getString(R.string.delete))
        .build();
  }

  @NonNull
  private Action buildDeleteAction()
  {
    return new Action.Builder()
        .setTitle(getCarContext().getString(R.string.delete))
        .setBackgroundColor(CarColor.RED)
        .setOnClickListener(this::onDeleteClicked)
        .build();
  }

  @NonNull
  private Action buildCancelAction()
  {
    return new Action.Builder()
        .setTitle(getCarContext().getString(R.string.cancel))
        .setOnClickListener(this::finish)
        .build();
  }

  private long totalSize()
  {
    long total = 0;
    for (final CountryItem map : mMaps)
      total += map.totalSize;
    return total;
  }

  @NonNull
  private String sizeStr(long bytes)
  {
    return StringUtils.getFileSizeString(getCarContext(), bytes);
  }
}
