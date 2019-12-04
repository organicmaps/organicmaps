protocol PlacePageReviewsViewControllerDelegate: AnyObject {
  func didPressMoreReviews()
}

class PlacePageReviewsViewController: UIViewController {
  @IBOutlet var stackView: UIStackView!

  var ugcData: UgcData? {
    didSet {
      updateReviews()
    }
  }
  weak var delegate: PlacePageReviewsViewControllerDelegate?

  lazy var myReviewViewController: MyReviewViewController = {
    let vc = storyboard!.instantiateViewController(ofType: MyReviewViewController.self)
    return vc
  } ()

  override func viewDidLoad() {
    super.viewDidLoad()
  }

  // MARK: private

  private func updateReviews() {
    guard let ugcData = ugcData else { return }

    if let myReview = ugcData.myReview {
      myReviewViewController.myReview = myReview
      addToStack(myReviewViewController)
    }

    for i in 0..<3 {
      if i < ugcData.reviews.count {
        let review = ugcData.reviews[i]
        let vc = storyboard!.instantiateViewController(ofType: PlacePageReviewViewController.self)
        vc.review = review
        addToStack(vc)
      } else {
        break
      }
    }

    if ugcData.reviews.count > 3 {
      createMoreReviewsButton()
    }
  }

  @objc private func onMoreReviewsButton(_ sender: UIButton) {
    delegate?.didPressMoreReviews()
  }

  private func createMoreReviewsButton() {
    let button = UIButton()
    button.setTitle(L("placepage_more_reviews_button"), for: .normal)
    button.titleLabel?.font = UIFont.regular17()
    button.setTitleColor(UIColor.linkBlue(), for: .normal)
    button.heightAnchor.constraint(equalToConstant: 44).isActive = true
    stackView.addArrangedSubview(button)
    button.addTarget(self, action: #selector(onMoreReviewsButton), for: .touchUpInside)
  }

  private func addToStack(_ viewController: UIViewController) {
    addChild(viewController)
    stackView.addArrangedSubview(viewController.view)
    viewController.didMove(toParent: self)
  }
}
