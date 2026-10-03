package app.organicmaps.car.screens.settings;

import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.StringRes;
import androidx.car.app.CarContext;
import androidx.car.app.model.Action;
import androidx.car.app.model.CarIcon;
import androidx.car.app.model.Header;
import androidx.car.app.model.Item;
import androidx.car.app.model.ItemList;
import androidx.car.app.model.ListTemplate;
import androidx.car.app.model.Row;
import androidx.car.app.model.Template;
import androidx.car.app.model.Toggle;
import androidx.car.app.navigation.model.MapWithContentTemplate;
import androidx.core.graphics.drawable.IconCompat;
import app.organicmaps.car.R;
import app.organicmaps.car.util.UiHelpers;
import app.organicmaps.sdk.OrganicMaps;
import app.organicmaps.sdk.car.renderer.Renderer;
import app.organicmaps.sdk.car.screens.BaseMapScreen;
import app.organicmaps.sdk.maplayer.Mode;
import java.util.List;

public class LayersScreen extends BaseMapScreen
{
  enum Layer
  {
    ISOLINES(Mode.ISOLINES, R.string.button_layer_isolines, R.drawable.ic_layers_isoline),
    SATELLITE(Mode.SATELLITE, R.string.button_layer_satellite, R.drawable.ic_layers_satellite);

    @NonNull
    private final Mode mLayerMode;
    @StringRes
    private final int mTitleResId;
    @DrawableRes
    private final int mIconResId;

    Layer(@NonNull Mode layerMode, @StringRes int titleResId, @DrawableRes int iconResId)
    {
      mLayerMode = layerMode;
      mTitleResId = titleResId;
      mIconResId = iconResId;
    }

    public boolean isAvailable()
    {
      return mLayerMode.isAvailable();
    }

    @NonNull
    public Mode getLayerMode()
    {
      return mLayerMode;
    }

    @StringRes
    public int getTitleResId()
    {
      return mTitleResId;
    }

    @DrawableRes
    public int getIconResId()
    {
      return mIconResId;
    }
  }

  public LayersScreen(@NonNull CarContext carContext, @NonNull OrganicMaps organicMapsContext,
                      @NonNull Renderer surfaceRenderer)
  {
    super(carContext, organicMapsContext, surfaceRenderer);
  }

  @NonNull
  @Override
  protected Template onGetTemplateImpl()
  {
    final MapWithContentTemplate.Builder builder = new MapWithContentTemplate.Builder();
    builder.setMapController(UiHelpers.createMapController(getCarContext(), getSurfaceRenderer(), getLocationHelper()));
    builder.setContentTemplate(createSettingsListTemplate());
    return builder.build();
  }

  @NonNull
  private Header createHeader()
  {
    final Header.Builder builder = new Header.Builder();
    builder.setStartHeaderAction(Action.BACK);
    builder.setTitle(getCarContext().getString(R.string.layers_title));
    return builder.build();
  }

  @NonNull
  private ListTemplate createSettingsListTemplate()
  {
    final ItemList.Builder builder = new ItemList.Builder();
    for (final Item item : createLayerItems())
      builder.addItem(item);
    return new ListTemplate.Builder().setHeader(createHeader()).setSingleList(builder.build()).build();
  }

  @NonNull
  private List<Item> createLayerItems()
  {
    final List<Item> items = new java.util.ArrayList<>();
    for (final Layer layer : Layer.values())
    {
      if (!layer.isAvailable())
        continue;

      final Toggle.OnCheckedChangeListener listener = (unused) ->
      {
        final boolean isEnabled = layer.getLayerMode().isEnabled(getCarContext());
        layer.getLayerMode().setEnabled(getCarContext(), !isEnabled);
        invalidate();
      };

      final Row.Builder builder = new Row.Builder();
      builder.setTitle(getCarContext().getString(layer.getTitleResId()));
      builder.setImage(
          new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), layer.getIconResId())).build());
      builder.setToggle(
          new Toggle.Builder(listener).setChecked(layer.getLayerMode().isEnabled(getCarContext())).build());
      items.add(builder.build());
    }
    return items;
  }
}
