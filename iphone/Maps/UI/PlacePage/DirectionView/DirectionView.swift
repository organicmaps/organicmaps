import UIKit

class DirectionView: SolidTouchView {
  @IBOutlet private var titleLabel: UILabel!
  @IBOutlet private var typeLabel: UILabel!
  @IBOutlet private var distanceLabel: UILabel!
  @IBOutlet private var directionArrow: UIImageView!
  @IBOutlet private var contentView: UIView!

  override func awakeFromNib() {
    super.awakeFromNib()
    directionArrow.isHidden = true
    distanceLabel.font = alternative(iPhone: .regular32, iPad: .regular52)
    typeLabel.font = alternative(iPhone: .regular16, iPad: .regular24)
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
  }

  func updateTitle(_ title: String?, subtitle: String?) {
    titleLabel.text = title
    typeLabel.text = subtitle
  }

  func updateDistance(_ distance: String?) {
    distanceLabel.text = distance
  }

  static func updateArrow(_ arrow: UIImageView, angle: CGFloat?, animated: Bool = true) {
    guard let angle else {
      arrow.isHidden = true
      // Clear any pending rotation so the arrow can reappear at the recovered angle.
      arrow.layer.removeAllAnimations()
      return
    }
    let animate = animated && !arrow.isHidden
    arrow.isHidden = false
    let transform = CGAffineTransform(rotationAngle: .pi / 2 - angle)
    if animate {
      UIView.animate(withDuration: AppConstants.defaultAnimationDuration,
                     delay: 0,
                     options: [.beginFromCurrentState, .curveEaseInOut]) {
        arrow.transform = transform
      }
    } else {
      arrow.transform = transform
    }
  }

  func updateHeading(_ angle: CGFloat?) {
    Self.updateArrow(directionArrow, angle: angle)
  }

  @IBAction func onTap(_: Any) {
    removeFromSuperview()
  }
}
