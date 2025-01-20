package app.organicmaps.products;

import androidx.annotation.Keep;
import androidx.annotation.Nullable;

// Called from JNI.
@Keep
@SuppressWarnings("unused")
public class Product {
    @Nullable
    public String title;

    @Nullable
    public String link;

    public Product(@Nullable String title, @Nullable String link) {
        this.title = title;
        this.link = link;
    }
}
