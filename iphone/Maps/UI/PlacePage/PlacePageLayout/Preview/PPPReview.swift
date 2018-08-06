@objc(MWMPPPReview)
final class PPPReview: MWMTableViewCell {
  @IBOutlet private weak var ratingSummaryView: RatingSummaryView!
  @IBOutlet private weak var reviewsLabel: UILabel! {
    didSet {
      reviewsLabel.font = UIFont.regular14()
      reviewsLabel.textColor = UIColor.blackSecondaryText()
    }
  }

  @IBOutlet private weak var pricingLabel: UILabel! {
    didSet {
      pricingLabel.font = UIFont.regular14()
    }
  }

  @IBOutlet private weak var addReviewButton: UIButton! {
    didSet {
      addReviewButton.backgroundColor = UIColor.linkBlue()
      addReviewButton.setTitle("+ \(L("leave_a_review"))", for: .normal)
      addReviewButton.setTitleColor(UIColor.white, for: .normal)
      addReviewButton.titleLabel?.font = UIFont.bold12()
    }
  }

  @IBOutlet private weak var discountView: UIView!
  @IBOutlet private weak var discountLabel: UILabel!
  @IBOutlet private weak var priceConstraint: NSLayoutConstraint!

  typealias OnAddReview = () -> ()
  private var onAddReview: OnAddReview?

  @objc func config(rating: UGCRatingValueType,
                    canAddReview: Bool,
                    reviewsCount: UInt,
                    price: String,
                    discount: Int,
                    smartDeal: Bool,
                    onAddReview: OnAddReview?) {
    self.onAddReview = onAddReview
    pricingLabel.text = price
    if discount > 0 {
      priceConstraint.priority = .defaultLow
      discountView.isHidden = false
      discountLabel.text = "-\(discount)%"
    } else if smartDeal {
      priceConstraint.priority = .defaultLow
      discountView.isHidden = false
      discountLabel.text = "%"
    } else {
      priceConstraint.priority = .defaultHigh
      discountView.isHidden = true
    }
    
    ratingSummaryView.textFont = UIFont.bold12()
    ratingSummaryView.value = rating.value
    ratingSummaryView.type = rating.type
    ratingSummaryView.backgroundOpacity = 0.05
    if canAddReview {
      addReviewButton.isHidden = false
      addReviewButton.layer.cornerRadius = addReviewButton.height / 2
    } else {
      addReviewButton.isHidden = true
    }
    pricingLabel.isHidden = true
    reviewsLabel.isHidden = false
    if rating.type == .noValue {
      if canAddReview {
        ratingSummaryView.noValueImage = #imageLiteral(resourceName: "ic_12px_rating_normal")
        ratingSummaryView.noValueColor = UIColor.blackSecondaryText()
        reviewsLabel.text = L("placepage_no_reviews")
      } else {
        ratingSummaryView.noValueImage = #imageLiteral(resourceName: "ic_12px_radio_on")
        ratingSummaryView.noValueColor = UIColor.linkBlue()
        reviewsLabel.text = L("placepage_reviewed")
        pricingLabel.isHidden = false
      }
    } else {
      ratingSummaryView.defaultConfig()

      if reviewsCount > 0 {
        reviewsLabel.text = String(format:L("placepage_summary_rating_description"), reviewsCount)
        reviewsLabel.isHidden = false
      } else {
        reviewsLabel.text = ""
        reviewsLabel.isHidden = true
      }
      pricingLabel.isHidden = false
    }
  }

  @IBAction private func addReview() {
    onAddReview?()
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    let inset = width / 2
    separatorInset = UIEdgeInsetsMake(0, inset, 0, inset)
  }
}
