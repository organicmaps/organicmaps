package app.organicmaps.products;

import androidx.annotation.Keep;
import androidx.annotation.Nullable;

// Called from JNI.
@Keep
@SuppressWarnings("unused")
public class ProductsConfig {
    public ProductsConfig(@Nullable String placePagePrompt, @Nullable Product[] products) {
        this.placePagePrompt = placePagePrompt;
        this.products = products;
    }

    @Nullable
    public String placePagePrompt;

    @Nullable
    public Product[] products;
}
