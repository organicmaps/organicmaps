class PlacePageReviewViewController: UIViewController {
  @IBOutlet var titleLabel: UILabel!
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

  var review: UgcReview!
  lazy var dateFormatter: DateFormatter = {
    let formatter = DateFormatter()
    formatter.dateStyle = .long
    formatter.timeStyle = .none
    return formatter
  } ()

  override func viewDidLoad() {
      super.viewDidLoad()

    titleLabel.text = review.author
    dateLabel.text = dateFormatter.string(from: review.date)
    expandableLabel.textLabel.text = review.text
  }
}
