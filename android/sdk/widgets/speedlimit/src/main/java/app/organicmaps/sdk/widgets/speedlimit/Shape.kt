package app.organicmaps.sdk.widgets.speedlimit

/**
 * Shape of a road sign, a square with rounded corners: the ratio of the corner radius to the sign
 * size tells the shapes apart, 0.5 making a circle.
 */
enum class Shape(val cornerRadiusRatio: Float) {
    Circle(0.5f),
    Square(0.2f),
}
