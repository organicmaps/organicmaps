let kExtendedTabBarTappableMargin: CGFloat = -15

final class BottomTabBarView: SolidTouchView {
  @IBOutlet var mainButtonsView: ExtendedBottomTabBarContainerView!

  override var placePageAreaAffectDirections: MWMAvailableAreaAffectDirections {
    alternative(iPhone: [], iPad: [.bottom])
  }

  override var widgetsAreaAffectDirections: MWMAvailableAreaAffectDirections {
    [.bottom]
  }

  override var sideButtonsAreaAffectDirections: MWMAvailableAreaAffectDirections {
    [.bottom]
  }

  override func point(inside point: CGPoint, with _: UIEvent?) -> Bool {
    bounds.insetBy(dx: kExtendedTabBarTappableMargin, dy: kExtendedTabBarTappableMargin).contains(point)
  }
}

final class ExtendedBottomTabBarContainerView: UIView {
  override func point(inside point: CGPoint, with _: UIEvent?) -> Bool {
    bounds.insetBy(dx: kExtendedTabBarTappableMargin, dy: kExtendedTabBarTappableMargin).contains(point)
  }
}
