final class PlacePageDirectionView: UIView {
  @IBOutlet var imageView: UIImageView!
  @IBOutlet var label: UILabel!

  var onAccessibilityActivate: (() -> Void)?

  override func accessibilityActivate() -> Bool {
    guard let onAccessibilityActivate else { return false }
    onAccessibilityActivate()
    return true
  }
}

final class PlacePagePreviewViewController: UIViewController {
  @IBOutlet var stackView: UIStackView!
  @IBOutlet var popularView: UIView!
  @IBOutlet var subtitleLabel: UILabel!
  @IBOutlet var subtitleContainerView: UIStackView!
  @IBOutlet var reviewsLabel: UILabel!
  @IBOutlet var addReviewButton: UIButton! {
    didSet {
      addReviewButton.setTitle("+ \(L("leave_a_review"))", for: .normal)
      addReviewButton.accessibilityLabel = L("leave_a_review")
    }
  }

  @IBOutlet var addressLabel: UILabel!
  @IBOutlet var addressContainerView: UIStackView!

  @IBOutlet var subtitleDirectionView: PlacePageDirectionView!
  @IBOutlet var addressDirectionView: PlacePageDirectionView!

  var placePageDirectionView: PlacePageDirectionView?
  lazy var fullScreenDirectionView: DirectionView = Bundle.main.load(viewClass: DirectionView.self)!

  var placePagePreviewData: PlacePagePreviewData! {
    didSet {
      if isViewLoaded {
        updateViews()
      }
    }
  }

  weak var delegate: PlacePageHeaderViewControllerDelegate?

  private var distance: String?
  private var speedAndAltitude: String?
  private var heading: CGFloat?

  override func viewDidLoad() {
    super.viewDidLoad()

    updateViews()

    subtitleLabel.adjustsFontForContentSizeCategory = true
    addressLabel.accessibilityCustomActions = [UIAccessibilityCustomAction(name: L("placepage_copy"), actionHandler: { [weak self] _ in
      guard let self, let address = self.addressLabel.text else { return false }
      self.delegate?.previewDidCopy(address)
      return true
    })]

    if let distance = distance {
      placePageDirectionView?.isHidden = false
      placePageDirectionView?.label.text = distance
    }

    if let heading = heading {
      updateHeading(heading)
    } else {
      placePageDirectionView?.imageView.isHidden = true
    }
  }

  func updateViews() {
    if placePagePreviewData.isMyPosition {
      if let speedAndAltitude = speedAndAltitude {
        subtitleLabel.text = speedAndAltitude
      }
    } else {
      let subtitleString = NSMutableAttributedString()
//      if placePagePreviewData.isPopular {
//        subtitleString.append(NSAttributedString(string: L("popular_place"),
//                                                 attributes: [.foregroundColor : UIColor.linkBlue,
//                                                              .font : UIFont.regular14]))
//      }

      if let subtitle = placePagePreviewData.subtitle ?? placePagePreviewData.coordinates {
        subtitleString.append(NSAttributedString(string: !subtitleString.string.isEmpty ? " • " + subtitle : subtitle,
                                                 attributes: [.foregroundColor: UIColor.blackSecondaryText,
                                                              .font: UIFont.emojiRegular14.dynamic]))

        subtitleLabel.attributedText = subtitleString
        subtitleContainerView.isHidden = false
      } else {
        subtitleLabel.text = nil
        subtitleContainerView.isHidden = true
      }
    }

    placePageDirectionView = subtitleDirectionView

    if let address = placePagePreviewData.secondarySubtitle {
      addressLabel.text = address
      placePageDirectionView = addressDirectionView
    } else {
      addressContainerView.isHidden = true
    }
    placePageDirectionView?.imageView.changeColoringToOpposite()
    updateDirectionAccessibility()
  }

  func updateDistance(_ distance: String) {
    self.distance = distance
    placePageDirectionView?.isHidden = false
    placePageDirectionView?.label.text = distance
    fullScreenDirectionView.updateDistance(distance)
    updateDirectionAccessibility()
  }

  func updateHeading(_ angle: CGFloat) {
    placePageDirectionView?.imageView.isHidden = false
    let duration = heading == nil ? .zero : AppConstants.defaultAnimationDuration // skip the initial setup animation
    UIView.animate(withDuration: duration,
                   delay: 0,
                   options: [.beginFromCurrentState, .curveEaseInOut],
                   animations: { [unowned self] in
                     placePageDirectionView?.imageView.transform = CGAffineTransform(rotationAngle: CGFloat.pi / 2 - angle)
                   })
    fullScreenDirectionView.updateHeading(angle)
    heading = angle
    updateDirectionAccessibility()
  }

  func updateSpeedAndAltitude(_ speedAndAltitude: String) {
    self.speedAndAltitude = speedAndAltitude
    subtitleLabel?.text = speedAndAltitude
  }

  @IBAction func onLongPressAdress(_ sender: UILongPressGestureRecognizer) {
    guard let address = addressLabel.text, sender.state == .began else { return }
    delegate?.previewDidCopy(address)
  }

  @IBAction func onDirectionPressed(_: Any) {
    guard let heading = heading else {
      return
    }

    fullScreenDirectionView.updateTitle(placePagePreviewData.title,
                                        subtitle: placePagePreviewData.subtitle ?? placePagePreviewData.coordinates)
    fullScreenDirectionView.updateHeading(heading)
    fullScreenDirectionView.updateDistance(distance)
    fullScreenDirectionView.accessibilityReturnFocus = placePageDirectionView
    fullScreenDirectionView.show()
  }

  private func updateDirectionAccessibility() {
    for directionView in [subtitleDirectionView, addressDirectionView] {
      directionView?.isAccessibilityElement = true
      directionView?.imageView.isAccessibilityElement = false
      directionView?.label.isAccessibilityElement = false
      directionView?.accessibilityLabel = L("placepage_distance")
      directionView?.accessibilityValue = distance
      directionView?.accessibilityTraits = heading == nil ? .staticText : .button
      directionView?.accessibilityHint = heading == nil ? nil : L("placepage_show_direction")
      directionView?.onAccessibilityActivate = heading == nil ? nil : { [weak self] in
        self?.onDirectionPressed(0)
      }
    }
  }
}
