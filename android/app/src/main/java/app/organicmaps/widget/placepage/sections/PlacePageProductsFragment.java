package app.organicmaps.widget.placepage.sections;

import static androidx.core.util.ObjectsCompat.requireNonNull;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

import java.util.Objects;

import app.organicmaps.Framework;
import app.organicmaps.R;
import app.organicmaps.products.Product;
import app.organicmaps.products.ProductsConfig;
import app.organicmaps.util.Constants;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.Utils;

public class PlacePageProductsFragment extends Fragment
{
  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.place_page_products_fragment, container, false);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    var config = Framework.nativeGetProductsConfiguration();
    if (config != null && isValidConfig(config))
    {
      UiUtils.show(view);

      updateView(view, config);
    } else
    {
      UiUtils.hide(view);
    }
  }

  private void updateView(@NonNull View view, @NonNull ProductsConfig config)
  {
    var layoutInflater = LayoutInflater.from(view.getContext());

    TextView productsPrompt = requireNonNull(view.findViewById(R.id.products_prompt));
    LinearLayout productsButtons = requireNonNull(view.findViewById(R.id.products_buttons));
    View closeButton = requireNonNull(view.findViewById(R.id.products_close));
    View productsRemindLater = requireNonNull(view.findViewById(R.id.products_remind_later));
    View alreadyDonated = requireNonNull(view.findViewById(R.id.products_already_donated));

    productsPrompt.setText(config.placePagePrompt);

    productsButtons.removeAllViews();

    for (var product : Objects.requireNonNull(config.products))
    {
      var button = (Button) layoutInflater.inflate(R.layout.item_product, productsButtons, false);
      button.setText(product.title);
      button.setOnClickListener((v) -> {
        onProductSelected(product);
      });

      productsButtons.addView(button);
    }

    closeButton.setOnClickListener((v) -> {
      closeWithReason(view, Constants.ProductsPopupCloseReason.CLOSE);
    });

    productsRemindLater.setOnClickListener((v) -> {
      closeWithReason(view, Constants.ProductsPopupCloseReason.REMIND_LATER);
    });

    alreadyDonated.setOnClickListener((v) -> {
      closeWithReason(view, Constants.ProductsPopupCloseReason.ALREADY_DONATED);
    });
  }

  private void closeWithReason(View view, String reason)
  {
    Framework.nativeDidCloseProductsPopup(reason);
    UiUtils.hide(view);
  }

  private void onProductSelected(Product product)
  {
    Utils.openUrl(requireActivity(), product.link);
    Framework.nativeDidSelectProduct(product.title, product.link);
  }

  private boolean isValidConfig(@NonNull ProductsConfig config)
  {
    return config.products != null && config.products.length > 0;
  }
}
