final class MyReviewCell: MWMTableViewCell {
  @IBOutlet var myReviewView: MyReviewView!

  override func awakeFromNib() {
    super.awakeFromNib()
    myReviewView.defaultConfig()
  }

  func config(_ myReview: UgcMyReview,
              formatter: DateFormatter,
              expanded: Bool,
              onExpand: @escaping ExpandableLabel.OnExpandClosure) {
    myReviewView.defaultConfig()
    myReviewView.reviewView.authorLabel.text = L("placepage_reviews_your_comment")
    myReviewView.reviewView.dateLabel.text = formatter.string(from: myReview.date)
    myReviewView.reviewView.reviewLabel.text = myReview.text
    myReviewView.reviewView.reviewLabel.numberOfLines = expanded ? 0 : 2
    myReviewView.reviewView.reviewLabel.onExpandClosure = onExpand

    for i in 0..<3 {
      if myReview.starRatings.count > i {
        let starRating = myReview.starRatings[i]
        myReviewView.ratingViews[i].nameLabel.text = L(starRating.title)
        myReviewView.ratingViews[i].starRatingView.rating = Int(round(starRating.value))
      } else {
        myReviewView.ratingViews[i].isHidden = true
      }
    }
  }
}

final class UserReviewCell: MWMTableViewCell {
  @IBOutlet var reviewView: ReviewView!

  override func awakeFromNib() {
    super.awakeFromNib()
    reviewView.defaultConfig()
  }

  func config(_ review: UgcReview, formatter: DateFormatter, expanded: Bool, onExpand: @escaping ExpandableLabel.OnExpandClosure) {
    reviewView.authorLabel.text = review.author
    reviewView.dateLabel.text = formatter.string(from: review.date)
    reviewView.reviewLabel.text = review.text
    reviewView.reviewLabel.numberOfLines = expanded ? 0 : 2
    reviewView.reviewLabel.onExpandClosure = onExpand
  }
}

final class MoreReviewsViewController: MWMTableViewController {
  private var expanded: [Bool] = []

  var ugcData: UgcData? {
    didSet {
      expanded = Array<Bool>(repeating: false, count: tableView.numberOfRows(inSection: 0))
    }
  }

  private lazy var dateFormatter: DateFormatter = {
    let formatter = DateFormatter()
    formatter.dateStyle = .long
    formatter.timeStyle = .none
    return formatter
  } ()

  override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    let reviewsCount = ugcData?.reviews.count ?? 0
    return reviewsCount + (ugcData?.myReview != nil ? 1 : 0);
  }

  override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    if let myReview = ugcData?.myReview, indexPath.item == 0 {
      let cell = tableView.dequeueReusableCell(withIdentifier: "MyReviewCell", for: indexPath) as! MyReviewCell
      cell.config(myReview, formatter: dateFormatter, expanded: expanded[indexPath.row]) { [unowned self] expandClosure in
        tableView.performBatchUpdates({
          self.expanded[indexPath.row] = true
          expandClosure()
        })
      }
      return cell
    }

    let cell = tableView.dequeueReusableCell(withIdentifier: "UserReviewCell", for: indexPath) as! UserReviewCell
    var index = indexPath.item - (ugcData?.myReview != nil ? 1 : 0)
    if index >= ugcData!.reviews.count {
      index = index % ugcData!.reviews.count
    }
    guard let review = ugcData?.reviews[index] else { fatalError() }
    cell.config(review, formatter: dateFormatter, expanded: expanded[indexPath.row]) { [unowned self] expandClosure in
      tableView.performBatchUpdates({
        self.expanded[indexPath.row] = true
        expandClosure()
      })
    }

    return cell
  }
}
