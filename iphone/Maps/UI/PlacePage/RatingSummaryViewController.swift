class RatingSummaryViewController: UIViewController {
  @IBOutlet var titleLabel: UILabel!
  @IBOutlet var countLabel: UILabel!
  @IBOutlet var ratingSummaryView: RatingSummaryView! {
    didSet {
      ratingSummaryView.horribleColor = UIColor.ratingRed()
      ratingSummaryView.badColor = UIColor.ratingOrange()
      ratingSummaryView.normalColor = UIColor.ratingYellow()
      ratingSummaryView.goodColor = UIColor.ratingLightGreen()
      ratingSummaryView.excellentColor = UIColor.ratingGreen()
      ratingSummaryView.textFont = UIFont.bold28()
      ratingSummaryView.textSize = 28
    }
  }
  @IBOutlet var ratingViews: [UIView]!
  @IBOutlet var ratingLabels: [UILabel]!
  @IBOutlet var starViews: [StarRatingView]! {
    didSet {
      starViews.forEach {
        $0.activeColor = UIColor.ratingYellow()
        $0.inactiveColor = UIColor.blackDividers()
      }
    }
  }
  
  var ugcData: UgcData? {
    didSet {
      updateRating()
    }
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    updateRating()
  }

  private func updateRating() {
    guard let ugcData = ugcData else { return }

    ratingSummaryView.value = ugcData.summaryRating!.ratingString
    ratingSummaryView.type = ugcData.summaryRating!.ratingType

    countLabel.text = String(format:L("placepage_summary_rating_description"), ugcData.ratingsCount)

    for i in 0..<3 {
      if ugcData.starRatings.count > i {
        let starRating = ugcData.starRatings[i]
        ratingLabels[i].text = L(starRating.title)
        starViews[i].rating = Int(round(starRating.value))
      } else {
        ratingViews[i].isHidden = true
      }
    }
  }
}
