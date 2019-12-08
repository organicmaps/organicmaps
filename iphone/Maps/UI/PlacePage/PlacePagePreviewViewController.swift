class DirectionView: UIView {
  @IBOutlet var button: UIButton!
}

protocol PlacePagePreviewViewControllerDelegate: AnyObject {
  func previewDidPressAddReview()
  func previewDidPressSimilarHotels()
}

class PlacePagePreviewViewController: UIViewController {
  @IBOutlet var titleLabel: UILabel!
  @IBOutlet var titleContainerView: UIStackView!
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

  @IBOutlet var titleDirectionView: DirectionView!
  @IBOutlet var subtitleDirectionView: DirectionView!
  @IBOutlet var addressDirectionView: DirectionView!

  var directionView: DirectionView?

  var placePagePreviewData: PlacePagePreviewData!
  weak var delegate: PlacePagePreviewViewControllerDelegate?

  override func viewDidLoad() {
    super.viewDidLoad()

    if let title = placePagePreviewData.title {
      titleLabel.text = title
      directionView = titleDirectionView
    } else {
      titleContainerView.isHidden = true
    }
    if let subtitle = placePagePreviewData.subtitle {
      subtitleLabel.text = subtitle
      directionView = subtitleDirectionView
    } else {
      subtitleContainerView.isHidden = true
    }
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
    popularView.isHidden = !placePagePreviewData.isPopular
    searchSimilarButton.isHidden = placePagePreviewData.hotelType == .none
    configSchedule()
    configUgc()
    ugcContainerView.isHidden = !placePagePreviewData.isBookingPlace

    directionView?.isHidden = false
//    directionView?.button.imageView?.transform = CGAffineTransform(rotationAngle: 0.2)
  }

  func updateUgc(_ ugcData: UgcData) {
    ugcContainerView.isHidden = false
    if let summaryRating = ugcData.summaryRating {
      ratingSummaryView.defaultConfig()
      ratingSummaryView.value = summaryRating.ratingString
      ratingSummaryView.type = summaryRating.ratingType
      reviewsLabel.text = String(format:L("placepage_summary_rating_description"), ugcData.ratingsCount)
    } else {
      if ugcData.isUpdateEmpty {
        ratingSummaryView.noValueImage = UIImage(named: "ic_12px_rating_normal")
        ratingSummaryView.noValueColor = UIColor.blackSecondaryText()
        reviewsLabel.text = ugcData.reviews.count == 0 ? L("placepage_no_reviews") : ""
      } else {
        ratingSummaryView.noValueImage = UIImage(named: "ic_12px_radio_on")
        ratingSummaryView.noValueColor = UIColor.linkBlue()
        reviewsLabel.text = L("placepage_reviewed")
        addReviewButton.isHidden = true
      }
    }

    addReviewButton.isHidden = !ugcData.isUpdateEmpty
  }

  func updateBooking(_ bookingData: HotelBookingData, rooms: HotelRooms?) {
    ugcContainerView.isHidden = false
    ratingSummaryView.defaultConfig()
    ratingSummaryView.value = NSNumber(value: bookingData.score).stringValue
    let rawRating = Int(bookingData.score / 2) + 1
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
    directionView?.button.setTitle(distance, for: .normal)
  }

  func updateHeading(_ angle: CGFloat) {
    UIView.animate(withDuration: kDefaultAnimationDuration, delay: 0, options: [.beginFromCurrentState, .curveEaseInOut], animations: {
      self.directionView?.button.transform = CGAffineTransform(rotationAngle: angle)
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
      scheduleLabel.isHidden = true
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
