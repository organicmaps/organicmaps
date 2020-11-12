class HotelReviewViewController: UIViewController {
  @IBOutlet var authorLabel: UILabel!
  @IBOutlet var dateLabel: UILabel!
  @IBOutlet var ratingLabel: UILabel!
  @IBOutlet var positiveView: UIView!
  @IBOutlet var positiveLabel: UILabel!
  @IBOutlet var negativeView: UIView!
  @IBOutlet var negativeLabel: UILabel!

  var review: HotelReview!

  override func viewDidLoad() {
    super.viewDidLoad()

    authorLabel.text = review.author
    ratingLabel.text = NSNumber(value: review.score).stringValue
    let formatter = DateFormatter()
    formatter.dateStyle = .long
    formatter.timeStyle = .none
    dateLabel.text = formatter.string(from: review.date)
    if let positiveReview = review.pros {
      positiveLabel.text = positiveReview
      positiveView.isHidden = false
    }

    if let negativeReview = review.cons {
      negativeLabel.text = negativeReview
      negativeView.isHidden = false
    }
  }
}

protocol HotelReviewsViewControllerDelegate: AnyObject {
  func hotelReviewsDidPressMore()
}

class HotelReviewsViewController: UIViewController {
  @IBOutlet var stackView: UIStackView!
  @IBOutlet var totalRatingLabel: UILabel!
  @IBOutlet var reviewsCountLabel: UILabel!

  var totalScore: Float = 0 {
    didSet {
      let ratingString: String
      switch totalScore {
      case 7..<8:
        ratingString = " (\(L("booking_filters_ragting_good")))"
      case 8..<9:
        ratingString = " (\(L("booking_filters_rating_very_good")))"
      case 9...Float.infinity:
        ratingString = " (\(L("booking_filters_rating_excellent")))"
      default:
        ratingString = ""
      }
      totalRatingLabel?.text = String(coreFormat:L("place_page_booking_rating"),
                                      arguments:[NSNumber(value: totalScore).stringValue]) + ratingString
    }
  }
  var reviewCount: UInt = 0 {
    didSet {
      reviewsCountLabel?.text = String(format:L("placepage_summary_rating_description"), reviewCount)
    }
  }
  var reviews: [HotelReview]? {
    didSet {
      updateReviews()
    }
  }

  weak var delegate: HotelReviewsViewControllerDelegate?

  override func viewDidLoad() {
    super.viewDidLoad()
    updateReviews()
  }

  private func updateReviews() {
    guard let reviews = reviews else { return }
    let count = min(reviews.count, 3)
    for i in 0..<count {
      addReview(reviews[i])
    }

    if reviewCount > count {
      addMoreButton()
    }
  }

  @objc func onMoreButton(_ sender: UIButton) {
    delegate?.hotelReviewsDidPressMore()
  }

  private func addReview(_ review: HotelReview) {
    let vc = storyboard!.instantiateViewController(ofType: HotelReviewViewController.self)
    vc.review = review
    addChild(vc)
    stackView.addArrangedSubview(vc.view)
    vc.didMove(toParent: self)
  }

  private func addMoreButton() {
    let button = UIButton()
    button.setTitle(L("reviews_on_bookingcom"), for: .normal)
    button.styleName = "MoreButton"
    button.heightAnchor.constraint(equalToConstant: 44).isActive = true
    stackView.addArrangedSubview(button)
    button.addTarget(self, action: #selector(onMoreButton(_:)), for: .touchUpInside)
  }
}
