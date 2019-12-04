protocol AddReviewViewControllerDelegate: AnyObject {
  func didRate(_ rating: UgcSummaryRatingType)
}

class AddReviewViewController: UIViewController {
  @IBOutlet var horribleButton: UIButton!
  @IBOutlet var badButton: UIButton!
  @IBOutlet var normalButton: UIButton!
  @IBOutlet var goodButton: UIButton!
  @IBOutlet var excellentButton: UIButton!

  weak var delegate: AddReviewViewControllerDelegate?

  override func viewDidLoad() {
    super.viewDidLoad()
  }
    
  @IBAction func onRate(_ sender: UIButton) {
    switch sender {
    case horribleButton:
      delegate?.didRate(.horrible)
    case badButton:
      delegate?.didRate(.bad)
    case normalButton:
      delegate?.didRate(.normal)
    case goodButton:
      delegate?.didRate(.good)
    case excellentButton:
      delegate?.didRate(.excellent)
    default:
      fatalError()
    }
  }
}
