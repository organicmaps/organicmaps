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

  // iPad: let touches in the wide gaps between the buttons reach the map,
  // keeping the extended button tappable areas. iPhone keeps swallowing touches:
  // its gaps are narrow and the strip above the home indicator is tall.
  override func hitTest(_ point: CGPoint, with event: UIEvent?) -> UIView? {
    let hitView = super.hitTest(point, with: event)
    guard isiPad else { return hitView }
    return hitView === self || hitView === mainButtonsView ? nil : hitView
  }
}

final class ExtendedBottomTabBarContainerView: UIView {
  override func point(inside point: CGPoint, with _: UIEvent?) -> Bool {
    bounds.insetBy(dx: kExtendedTabBarTappableMargin, dy: kExtendedTabBarTappableMargin).contains(point)
  }
}
