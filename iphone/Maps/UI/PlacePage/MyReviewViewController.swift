class MyReviewViewController: UIViewController {
  @IBOutlet var dateLabel: UILabel!
  @IBOutlet var expandableLabel: ExpandableLabel! {
    didSet {
      expandableLabel.textLabel.font = UIFont.regular14()
      expandableLabel.textLabel.textColor = UIColor.blackPrimaryText()
      expandableLabel.expandButton.setTitleColor(UIColor.linkBlue(), for: .normal)
      expandableLabel.expandButton.titleLabel?.font = UIFont.regular14()
      expandableLabel.expandButton.setTitle(L("placepage_more_button"), for: .normal)
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

  var myReview: UgcMyReview!
  lazy var dateFormatter: DateFormatter = {
    let formatter = DateFormatter()
    formatter.dateStyle = .long
    formatter.timeStyle = .none
    return formatter
  } ()

  override func viewDidLoad() {
    super.viewDidLoad()

    dateLabel.text = dateFormatter.string(from: myReview.date)
    expandableLabel.textLabel.text = myReview.text

    for i in 0..<3 {
      if myReview.starRatings.count > i {
        let starRating = myReview.starRatings[i]
        ratingLabels[i].text = L(starRating.title)
        starViews[i].rating = Int(round(starRating.value))
      } else {
        ratingViews[i].isHidden = true
      }
    }
  }
}
