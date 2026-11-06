final class PlacePageDirectionView: UIView {
  @IBOutlet var imageView: UIImageView!
  @IBOutlet var label: UILabel!
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
  }

  func updateDistance(_ distance: String) {
    self.distance = distance
    placePageDirectionView?.isHidden = false
    placePageDirectionView?.label.text = distance
    fullScreenDirectionView.updateDistance(distance)
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
    fullScreenDirectionView.show()
  }
}
