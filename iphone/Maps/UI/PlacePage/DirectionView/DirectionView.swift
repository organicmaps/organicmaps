import UIKit

class DirectionView: SolidTouchView {
  @IBOutlet private var titleLabel: UILabel!
  @IBOutlet private var typeLabel: UILabel!
  @IBOutlet private var distanceLabel :UILabel!
  @IBOutlet private var directionArrow: UIImageView!
  @IBOutlet private var contentView: UIView!

  override func awakeFromNib() {
    distanceLabel.font = alternative(iPhone: .regular32(), iPad: .regular52())
    typeLabel.font = alternative(iPhone: .regular16(), iPad: .regular24())
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
    self.alignToSuperview()
    setNeedsLayout()
  }

  func updateTitle(_ title: String?, subtitle: String?) {
    self.titleLabel.text = title
    self.typeLabel.text = subtitle
  }

  func updateDistance(_ distance: String?) {
    distanceLabel?.text = distance
  }

  func updateHeading(_ angle: CGFloat) {
    UIView.animate(withDuration: kDefaultAnimationDuration,
                   delay: 0,
                   options: [.beginFromCurrentState, .curveEaseInOut],
                   animations: { [unowned self] in
                    self.directionArrow?.transform = CGAffineTransform(rotationAngle: CGFloat.pi / 2 - angle)
    })
  }

  @IBAction func onTap(_ sender: Any) {
    removeFromSuperview()
  }
}
