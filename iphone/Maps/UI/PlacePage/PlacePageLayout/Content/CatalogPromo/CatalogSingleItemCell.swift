@objc class CatalogSingleItemCell: UITableViewCell {
  @IBOutlet var placeTitleLabel: UILabel!
  @IBOutlet var placeDescriptionLabel: UILabel!
  @IBOutlet var guideNameLabel: UILabel!
  @IBOutlet var guideAuthorLabel: UILabel!
  @IBOutlet var moreButton: UIButton!
  @IBOutlet var placeImageView: UIImageView!
  @IBOutlet var openCatalogButton: UIButton!
  @IBOutlet var moreButtonHeightConstraint: NSLayoutConstraint!
  @IBOutlet var guideContainerView: UIView!
  @IBOutlet var headerLabel: UILabel!

  @objc var onMore: MWMVoidBlock?
  @objc var onView: MWMVoidBlock?

  override func awakeFromNib() {
    super.awakeFromNib()
    guideContainerView.layer.borderColor = UIColor.blackDividers().cgColor
    headerLabel.text = L("pp_discovery_place_related_header").uppercased()
  }

  @objc func config(_ promoItem: CatalogPromoItem) {
    placeTitleLabel.text = promoItem.placeTitle
    placeDescriptionLabel.text = promoItem.placeDescription
    guideNameLabel.text = promoItem.guideName
    guideAuthorLabel.text = promoItem.guideAuthor
    guard let url = URL(string: promoItem.imageUrl) else {
      return
    }
    placeImageView.wi_setImage(with: url)
  }
  
  @IBAction func onMoreButton(_ sender: UIButton) {
    onMore?()
  }

  @IBAction func onCatalogButton(_ sender: UIButton) {
    onView?()
  }
}
