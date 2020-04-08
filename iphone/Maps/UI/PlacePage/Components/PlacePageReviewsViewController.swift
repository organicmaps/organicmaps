protocol PlacePageReviewsViewControllerDelegate: AnyObject {
  func didPressMoreReviews()
}

final class PlacePageReviewsViewController: UIViewController {
  @IBOutlet var stackView: UIStackView!

  var ugcData: UgcData? {
    didSet {
      updateReviews()
    }
  }
  
  weak var delegate: PlacePageReviewsViewControllerDelegate?

  // MARK: private

  private lazy var dateFormatter: DateFormatter = {
    let formatter = DateFormatter()
    formatter.dateStyle = .long
    formatter.timeStyle = .none
    return formatter
  } ()

  private func updateReviews() {
    guard let ugcData = ugcData else { return }

    stackView.arrangedSubviews.forEach {
      stackView.removeArrangedSubview($0)
      $0.removeFromSuperview()
    }

    if let myReview = ugcData.myReview {
      let myReviewView = MyReviewView()
      myReviewView.defaultConfig()
      myReviewView.reviewView.authorLabel.text = L("placepage_reviews_your_comment")
      myReviewView.reviewView.dateLabel.text = dateFormatter.string(from: myReview.date)
      myReviewView.reviewView.reviewLabel.text = myReview.text

      for i in 0..<3 {
        if myReview.starRatings.count > i {
          let starRating = myReview.starRatings[i]
          myReviewView.ratingViews[i].nameLabel.text = L(starRating.title)
          myReviewView.ratingViews[i].starRatingView.rating = Int(round(starRating.value))
        } else {
          myReviewView.ratingViews[i].isHidden = true
        }
      }
      stackView.addArrangedSubview(myReviewView)
    }

    for i in 0..<3 {
      if i < ugcData.reviews.count {
        let review = ugcData.reviews[i]
        addReviewView(review)
      } else {
        break
      }
    }

    if ugcData.reviews.count > 3 {
      createMoreReviewsButton()
    }
  }

  private func addReviewView(_ review: UgcReview) {
    let reviewView = ReviewView()
    reviewView.defaultConfig()
    reviewView.authorLabel.text = review.author
    reviewView.dateLabel.text = dateFormatter.string(from: review.date)
    reviewView.reviewLabel.text = review.text

    stackView.addArrangedSubview(reviewView)
  }

  @objc private func onMoreReviewsButton(_ sender: UIButton) {
    delegate?.didPressMoreReviews()
  }

  private func createMoreReviewsButton() {
    let button = UIButton()
    button.setTitle(L("placepage_more_reviews_button"), for: .normal)
    button.styleName = "MoreButton"
    button.heightAnchor.constraint(equalToConstant: 44).isActive = true
    stackView.addArrangedSubview(button)
    button.addTarget(self, action: #selector(onMoreReviewsButton), for: .touchUpInside)
  }
}
