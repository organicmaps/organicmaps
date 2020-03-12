class DirectionView: UIView {
  @IBOutlet var imageView: UIImageView!
  @IBOutlet var label: UILabel!
}

protocol PlacePagePreviewViewControllerDelegate: AnyObject {
  func previewDidPressAddReview()
  func previewDidPressSimilarHotels()
  func previewDidPressRemoveAds()
}

class PlacePagePreviewViewController: UIViewController {
  @IBOutlet var stackView: UIStackView!
  @IBOutlet var popularView: UIView!
  @IBOutlet var subtitleLabel: UILabel!
  @IBOutlet var subtitleContainerView: UIStackView!
  @IBOutlet var scheduleLabel: UILabel!
  @IBOutlet var ratingSummaryView: RatingSummaryView!
  @IBOutlet var reviewsLabel: UILabel!
  @IBOutlet var addReviewButton: UIButton! {
    didSet {
      addReviewButton.setTitle("+ \(L("leave_a_review"))", for: .normal)
    }
  }
  @IBOutlet var priceLabel: UILabel!
  @IBOutlet var discountView: UIView!
  @IBOutlet var discountLabel: UILabel!
  @IBOutlet var ugcContainerView: UIStackView!
  @IBOutlet var addressLabel: UILabel!
  @IBOutlet var addressContainerView: UIStackView!
  @IBOutlet var searchSimilarButton: UIButton!
  @IBOutlet var scheduleContainerView: UIStackView!
  @IBOutlet var searchSimilarContainerView: UIStackView!

  @IBOutlet var subtitleDirectionView: DirectionView!
  @IBOutlet var addressDirectionView: DirectionView!

  var directionView: DirectionView?
  lazy var adView: AdBannerView = {
    let view = Bundle.main.load(viewClass: AdBannerView.self)?.first as! AdBannerView
    view.isHidden = true
    return view
  }()

  var placePagePreviewData: PlacePagePreviewData!
  weak var delegate: PlacePagePreviewViewControllerDelegate?

  private var distance: String? = nil
  private var heading: CGFloat? = nil

  override func viewDidLoad() {
    super.viewDidLoad()
    let subtitleString = NSMutableAttributedString()
    if placePagePreviewData.isPopular {
      subtitleString.append(NSAttributedString(string: L("popular_place"),
                                               attributes: [.foregroundColor : UIColor.linkBlue(),
                                                            .font : UIFont.regular14()]))
    }

    if let subtitle = placePagePreviewData.subtitle ?? placePagePreviewData.coordinates {
      subtitleString.append(NSAttributedString(string: placePagePreviewData.isPopular ? " • " + subtitle : subtitle,
                                               attributes: [.foregroundColor : UIColor.blackSecondaryText(),
                                                            .font : UIFont.regular14()]))
    }

    directionView = subtitleDirectionView
    subtitleLabel.attributedText = subtitleString

    if let address = placePagePreviewData.address {
      addressLabel.text = address
      directionView = addressDirectionView
    } else {
      addressContainerView.isHidden = true
    }

    if let pricing = placePagePreviewData.pricing {
      priceLabel.text = pricing
    } else {
      priceLabel.isHidden = true
    }
    searchSimilarContainerView.isHidden = placePagePreviewData.hotelType == .none
    configSchedule()
    configUgc()
    ugcContainerView.isHidden = !placePagePreviewData.isBookingPlace

    if let distance = distance {
      directionView?.isHidden = false
      directionView?.label.text = distance
    }

    if let heading = heading {
      updateHeading(heading)
    } else {
      directionView?.imageView.isHidden = true
    }

    stackView.addArrangedSubview(adView)
  }

  func updateBanner(_ banner: MWMBanner) {
    adView.isHidden = false
    adView.config(ad: banner, containerType: .placePage, canRemoveAds: true) { [weak self] in
      self?.delegate?.previewDidPressRemoveAds()
    }
  }

  func updateUgc(_ ugcData: UgcData) {
    ugcContainerView.isHidden = false
    if let summaryRating = ugcData.summaryRating {
      ratingSummaryView.value = summaryRating.ratingString
      ratingSummaryView.type = summaryRating.ratingType
      reviewsLabel.text = String(format:L("placepage_summary_rating_description"), ugcData.ratingsCount)
    } else {
      if ugcData.isUpdateEmpty {
        ratingSummaryView.setStyleAndApply("RatingSummaryView12")
        reviewsLabel.text = ugcData.reviews.count == 0 ? L("placepage_no_reviews") : ""
      } else {
        ratingSummaryView.setStyleAndApply("RatingSummaryView12User")
        reviewsLabel.text = L("placepage_reviewed")
        addReviewButton.isHidden = true
      }
    }

    addReviewButton.isHidden = !ugcData.isUpdateEmpty
  }

  func updateBooking(_ bookingData: HotelBookingData, rooms: HotelRooms?) {
    var rawRating: Int
    switch bookingData.score {
    case 0..<2:
      rawRating = 1
    case 2..<4:
      rawRating = 2
    case 4..<6:
      rawRating = 3
    case 6..<8:
      rawRating = 4
    case 8...10:
      rawRating = 5
    default:
      rawRating = 0
    }

    ugcContainerView.isHidden = false
    ratingSummaryView.value = NSNumber(value: bookingData.score).stringValue
    ratingSummaryView.type = UgcSummaryRatingType(rawValue: rawRating) ?? .none
    guard let rooms = rooms else { return }
    priceLabel.text = String(coreFormat: L("place_page_starting_from"), arguments: [rooms.minPrice])
    priceLabel.isHidden = false
    if rooms.discount > 0 {
      discountLabel.text = "-\(rooms.discount)%"
      discountView.isHidden = false
    } else if rooms.isSmartDeal {
      discountLabel.text = "%"
      discountView.isHidden = false
    }
  }

  func updateDistance(_ distance: String) {
    self.distance = distance
    directionView?.isHidden = false
    directionView?.label.text = distance
  }

  func updateHeading(_ angle: CGFloat) {
    heading = angle
    directionView?.imageView.isHidden = false
    UIView.animate(withDuration: kDefaultAnimationDuration,
                   delay: 0,
                   options: [.beginFromCurrentState, .curveEaseInOut],
                   animations: { [unowned self] in
                    self.directionView?.imageView.transform = CGAffineTransform(rotationAngle: CGFloat.pi / 2 - angle)
    })
  }

  @IBAction func onAddReview(_ sender: UIButton) {
    delegate?.previewDidPressAddReview()
  }

  @IBAction func onSimilarHotels(_ sender: UIButton) {
    delegate?.previewDidPressSimilarHotels()
  }
  // MARK: private

  private func configSchedule() {
    switch placePagePreviewData.schedule {
    case .openingHoursAllDay:
      scheduleLabel.text = L("twentyfour_seven")
    case .openingHoursOpen:
      scheduleLabel.text = L("editor_time_open")
    case .openingHoursClosed:
      scheduleLabel.text = L("closed_now")
      scheduleLabel.textColor = UIColor.red
    case .openingHoursUnknown:
      scheduleContainerView.isHidden = true
    @unknown default:
      fatalError()
    }
  }

  private func configUgc() {
    ratingSummaryView.textFont = UIFont.bold12()
    ratingSummaryView.backgroundOpacity = 0.05
    ratingSummaryView.value = "-"

    if placePagePreviewData.isBookingPlace {
      reviewsLabel.isHidden = true
      addReviewButton.isHidden = true
    } else {
      priceLabel.isHidden = true
      discountView.isHidden = true
    }
  }
}
