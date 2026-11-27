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
import androidx.core.view.AccessibilityDelegateCompat;
import androidx.core.view.ViewCompat;
import androidx.core.view.accessibility.AccessibilityNodeInfoCompat;
import androidx.fragment.app.Fragment;
import app.organicmaps.R;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.products.Product;
import app.organicmaps.sdk.products.ProductsConfig;
import app.organicmaps.sdk.util.Constants;
import app.organicmaps.util.MultiTouchDelegate;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.Utils;
import java.util.Objects;

public class PlacePageProductsFragment extends Fragment
{
  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
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
    }
    else
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

    attachActionTargets((ViewGroup) view);
    productsPrompt.setText(config.placePagePrompt);

    productsButtons.removeAllViews();

    for (var product : Objects.requireNonNull(config.products))
    {
      var button = (Button) layoutInflater.inflate(R.layout.item_product, productsButtons, false);
      button.setText(product.title);
      button.setOnClickListener((v) -> { onProductSelected(view, product); });

      productsButtons.addView(button);
    }

    closeButton.setOnClickListener((v) -> { closeWithReason(view, Constants.ProductsPopupCloseReason.CLOSE); });

    productsRemindLater.setOnClickListener(
        (v) -> { closeWithReason(view, Constants.ProductsPopupCloseReason.REMIND_LATER); });

    alreadyDonated.setOnClickListener(
        (v) -> { closeWithReason(view, Constants.ProductsPopupCloseReason.ALREADY_DONATED); });
  }

  static void attachActionTargets(ViewGroup host)
  {
    for (int id : new int[] {R.id.products_close, R.id.products_remind_later, R.id.products_already_donated})
    {
      View target = host.findViewById(id);
      target.setFocusable(true);
      ViewCompat.setAccessibilityDelegate(target, new AccessibilityDelegateCompat() {
        @Override
        public void onInitializeAccessibilityNodeInfo(View view, AccessibilityNodeInfoCompat info)
        {
          super.onInitializeAccessibilityNodeInfo(view, info);
          info.setClassName(Button.class.getName());
        }
      });
      target.addOnLayoutChangeListener(
          (v, left, top, right, bottom, oldLeft, oldTop, oldRight, oldBottom) -> expandTarget(host, v));
      if (target.isLaidOut())
        expandTarget(host, target);
    }
  }

  private static void expandTarget(ViewGroup host, View target)
  {
    int minimum = Math.round(48 * target.getResources().getDisplayMetrics().density);
    int horizontal = Math.max(0, (minimum - target.getWidth() + 1) / 2);
    int vertical = Math.max(0, (minimum - target.getHeight() + 1) / 2);
    MultiTouchDelegate.expand(host, target, vertical, horizontal, vertical, horizontal);
  }

  private void closeWithReason(View view, String reason)
  {
    Framework.nativeDidCloseProductsPopup(reason);
    UiUtils.hide(view);
  }

  private void onProductSelected(@NonNull View view, @NonNull Product product)
  {
    Utils.openUrl(requireActivity(), product.link);
    // Also updates the donation page stats used by the crowdfunding promo.
    Framework.nativeDidSelectProduct(product.title, product.link);
    // Start the "product selected" cooldown and hide the section, like on iOS.
    closeWithReason(view, Constants.ProductsPopupCloseReason.SELECT_PRODUCT);
  }

  private boolean isValidConfig(@NonNull ProductsConfig config)
  {
    return config.products != null && config.products.length > 0;
  }
}
