import UIKit

class DirectionView: SolidTouchView {
  @IBOutlet private var titleLabel: UILabel!
  @IBOutlet private var typeLabel: UILabel!
  @IBOutlet private var distanceLabel: UILabel!
  @IBOutlet private var directionArrow: UIImageView!
  @IBOutlet private var contentView: UIView!
  weak var accessibilityReturnFocus: UIView?

  override func awakeFromNib() {
    distanceLabel.font = alternative(iPhone: .regular32, iPad: .regular52)
    typeLabel.font = alternative(iPhone: .regular16, iPad: .regular24)
    isAccessibilityElement = true
    accessibilityTraits = .button
    accessibilityHint = L("close")
    directionArrow.isAccessibilityElement = false
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

  override func layoutSubviews() {
    var textAlignment = NSTextAlignment.center
    if UIDevice.current.orientation == .landscapeLeft || UIDevice.current.orientation == .landscapeRight {
      textAlignment = alternative(iPhone: .left, iPad: .center)
    }
    titleLabel.textAlignment = textAlignment
    typeLabel.textAlignment = textAlignment
    distanceLabel.textAlignment = textAlignment

    super.layoutSubviews()
  }

  func show() {
    guard let superview = MapViewController.shared()?.view else {
      assertionFailure()
      return
    }
    superview.addSubview(self)
    alignToSuperview()
    setNeedsLayout()
    if UIAccessibility.isVoiceOverRunning {
      UIAccessibility.post(notification: .screenChanged, argument: self)
    }
  }

  func updateTitle(_ title: String?, subtitle: String?) {
    titleLabel.text = title
    typeLabel.text = subtitle
    updateAccessibilityLabel()
  }

  func updateDistance(_ distance: String?) {
    distanceLabel?.text = distance
    updateAccessibilityLabel()
  }

  func updateHeading(_ angle: CGFloat) {
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

  override func accessibilityActivate() -> Bool {
    close()
    return true
  }

  override func accessibilityPerformEscape() -> Bool {
    close()
    return true
  }

  private func close() {
    removeFromSuperview()
    if UIAccessibility.isVoiceOverRunning {
      UIAccessibility.post(notification: .screenChanged, argument: accessibilityReturnFocus)
    }
  }

  private func updateAccessibilityLabel() {
    accessibilityLabel = [titleLabel.text, typeLabel.text, distanceLabel.text]
      .compactMap { $0?.isEmpty == false ? $0 : nil }
      .joined(separator: ", ")
  }
}
