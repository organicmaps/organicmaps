import CarPlay

final class DashboardBuilder {
  class func buildShortcutButtons(isMapOnPhone: Bool,
                                  openCarPlay: @escaping () -> Void) -> [CPDashboardButton] {
    if isMapOnPhone {
      return [
        CPDashboardButton(
          titleVariants: [L("car_continue_in_the_car")],
          subtitleVariants: [],
          image: UIImage.icInfo,
          handler: { _ in openCarPlay() }
        ),
      ]
    }

    return [
      CPDashboardButton(
        titleVariants: [L("zoom_in")],
        subtitleVariants: [],
        image: UIImage.btnZoomIn,
        handler: { _ in FrameworkHelper.zoomMap(.in) }
      ),
      CPDashboardButton(
        titleVariants: [L("zoom_out")],
        subtitleVariants: [],
        image: UIImage.btnZoomOut,
        handler: { _ in FrameworkHelper.zoomMap(.out) }
      ),
    ]
  }
}
