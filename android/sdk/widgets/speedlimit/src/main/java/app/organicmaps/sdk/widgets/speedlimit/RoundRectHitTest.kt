package app.organicmaps.sdk.widgets.speedlimit

import android.graphics.RectF

/**
 * Tests whether the point is inside the rounded rectangle.
 *
 * Used for hit testing, so that touches in the corners, which are not covered by the shape,
 * are not consumed by the widget and are dispatched further, to the map below.
 */
internal fun isInsideRoundRect(bounds: RectF, radius: Float, x: Float, y: Float): Boolean {
    if (!bounds.contains(x, y)) {
        return false
    }

    // Distance from the point to the center of the nearest rounded corner. Non-positive on any
    // axis means the point is in the straight part of the shape.
    val dx = (bounds.left + radius - x).coerceAtLeast(x - (bounds.right - radius))
    val dy = (bounds.top + radius - y).coerceAtLeast(y - (bounds.bottom - radius))
    if (dx <= 0f || dy <= 0f) {
        return true
    }
    return dx * dx + dy * dy <= radius * radius
}
