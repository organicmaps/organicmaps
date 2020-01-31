protocol CatalogSingleItemViewControllerDelegate: AnyObject {
  func catalogPromoItemDidPressView()
  func catalogPromoItemDidPressMore()
}

class CatalogSingleItemViewController: UIViewController {
  @IBOutlet var placeTitleLabel: UILabel!
  @IBOutlet var placeDescriptionLabel: UILabel!
  @IBOutlet var guideNameLabel: UILabel!
  @IBOutlet var guideAuthorLabel: UILabel!
  @IBOutlet var placeImageView: UIImageView!
  @IBOutlet var headerLabel: UILabel!

  var promoItem: CatalogPromoItem? {
    didSet {
      updateViews()
    }
  }
  weak var delegate: CatalogSingleItemViewControllerDelegate?

  override func viewDidLoad() {
    super.viewDidLoad()
    headerLabel.text = L("pp_discovery_place_related_header").uppercased()
    updateViews()
  }

  private func updateViews() {
    guard let promoItem = promoItem else { return }
    placeTitleLabel.text = promoItem.placeTitle
    placeDescriptionLabel.text = promoItem.placeDescription
    guideNameLabel.text = promoItem.guideName
    guideAuthorLabel.text = promoItem.guideAuthor
    guard let url = URL(string: promoItem.imageUrl) else {
      return
    }
    placeImageView.wi_setImage(with: url)
  }

  @IBAction func onMore(_ sender: UIButton) {
    delegate?.catalogPromoItemDidPressMore()
  }

  @IBAction func onView(_ sender: UIButton) {
    delegate?.catalogPromoItemDidPressView()
  }
}
