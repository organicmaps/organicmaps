import UIKit

class DirectionView: SolidTouchView {
  @IBOutlet private var titleLabel: UILabel!
  @IBOutlet private var typeLabel: UILabel!
  @IBOutlet private(set) var distanceLabel: UILabel!
  @IBOutlet private(set) var bearingLabel: UILabel!
  @IBOutlet private(set) var measurementsStack: UIStackView!
  @IBOutlet private(set) var directionArrow: UIImageView!

  override func awakeFromNib() {
    super.awakeFromNib()
    distanceLabel.font = alternative(iPhone: .regular32, iPad: .regular52)
    bearingLabel.font = distanceLabel.font
    typeLabel.font = alternative(iPhone: .regular16, iPad: .regular24)
    updateLayout()
  }

  override func didMoveToSuperview() {
    super.didMoveToSuperview()
    let manager = MWMMapViewControlsManager.manager()
    let app = MapsAppDelegate.theApp()
    if superview != nil {
      app.disableStandby()
      manager?.isDirectionViewHidden = false
    } else {
      app.enableStandby()
      manager?.isDirectionViewHidden = true
    }
    app.mapViewController.updateStatusBarStyle()
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    guard measurementsStack != nil,
          previousTraitCollection?.horizontalSizeClass != traitCollection.horizontalSizeClass ||
          previousTraitCollection?.verticalSizeClass != traitCollection.verticalSizeClass else { return }
    updateLayout()
  }

  private func updateLayout() {
    let landscape = traitCollection.verticalSizeClass == .compact
    let horizontal = landscape || traitCollection.horizontalSizeClass == .regular
    measurementsStack.axis = horizontal ? .horizontal : .vertical
    measurementsStack.alignment = horizontal ? .firstBaseline : .fill
    measurementsStack.spacing = horizontal ? 16 : 8
    let textAlignment: NSTextAlignment = landscape ? alternative(iPhone: .left, iPad: .center) : .center
    titleLabel.textAlignment = textAlignment
    typeLabel.textAlignment = textAlignment
    distanceLabel.textAlignment = textAlignment
    bearingLabel.textAlignment = textAlignment
  }

  func show() {
    guard let superview = MapViewController.shared()?.view else {
      assertionFailure()
      return
    }
    superview.addSubview(self)
    alignToSuperview()
    setNeedsLayout()
  }

  func updateTitle(_ title: String?, subtitle: String?) {
    titleLabel.text = title
    typeLabel.text = subtitle
  }

  func updateDistance(_ distance: String?) {
    distanceLabel?.text = distance
  }

  func updateBearing(_ bearing: Int) {
    bearingLabel.text = "\(bearing)°"
  }

  func updateHeading(_ angle: CGFloat?) {
    directionArrow.isHidden = angle == nil
    guard let angle else { return }
    UIView.animate(withDuration: AppConstants.defaultAnimationDuration,
                   delay: 0,
                   options: [.beginFromCurrentState, .curveEaseInOut],
                   animations: { [unowned self] in
                     directionArrow?.transform = CGAffineTransform(rotationAngle: CGFloat.pi / 2 - angle)
                   })
  }

  @IBAction func onTap(_: Any) {
    removeFromSuperview()
  }
}
