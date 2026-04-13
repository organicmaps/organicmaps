// Preserves `Graphics.tint(...)` static call sites in the remaining Java callers.
// Without @file:JvmName, Kotlin generates "GraphicsKt" for top-level functions.
@file:JvmName("Graphics")

package app.organicmaps.util

import android.content.Context
import android.graphics.drawable.Drawable
import android.widget.TextView
import androidx.annotation.AttrRes
import androidx.annotation.DrawableRes
import androidx.appcompat.content.res.AppCompatResources
import app.organicmaps.R
import app.organicmaps.sdk.util.Graphics

@JvmOverloads
fun TextView.tint(@AttrRes tintAttr: Int = R.attr.iconTint) {
    val d = compoundDrawablesRelative
    setCompoundDrawablesRelativeWithIntrinsicBounds(
        context.tint(d[0], tintAttr),
        context.tint(d[1], tintAttr),
        context.tint(d[2], tintAttr),
        context.tint(d[3], tintAttr),
    )
}

@JvmOverloads
fun Context.tint(@DrawableRes resId: Int, @AttrRes tintAttr: Int = R.attr.iconTint): Drawable? =
    tint(AppCompatResources.getDrawable(this, resId), tintAttr)

@JvmOverloads
fun Context.tint(drawable: Drawable?, @AttrRes tintAttr: Int = R.attr.iconTint): Drawable? =
    Graphics.tint(drawable, ThemeUtils.getColor(this, tintAttr))
