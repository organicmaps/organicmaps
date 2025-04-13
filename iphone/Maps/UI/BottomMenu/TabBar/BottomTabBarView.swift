let kExtendedTabBarTappableMargin: CGFloat = -15

final class BottomTabBarView: SolidTouchView {

  @IBOutlet var mainButtonsView: ExtendedBottomTabBarContainerView!

  override var placePageAreaAffectDirections: MWMAvailableAreaAffectDirections {
    return alternative(iPhone: [], iPad: [.bottom])
  }

  override var widgetsAreaAffectDirections: MWMAvailableAreaAffectDirections {
    return [.bottom]
  }

  override var sideButtonsAreaAffectDirections: MWMAvailableAreaAffectDirections {
    return [.bottom]
  }

  override func point(inside point: CGPoint, with event: UIEvent?) -> Bool {
    return bounds.insetBy(dx: kExtendedTabBarTappableMargin, dy: kExtendedTabBarTappableMargin).contains(point)
  }
}

final class ExtendedBottomTabBarContainerView: UIView {
  override func point(inside point: CGPoint, with event: UIEvent?) -> Bool {
    return bounds.insetBy(dx: kExtendedTabBarTappableMargin, dy: kExtendedTabBarTappableMargin).contains(point)
  }
}
