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

  @objc var onMore: MWMVoidBlock?
  @objc var onGoToCatalog: MWMVoidBlock?

  override func awakeFromNib() {
    super.awakeFromNib()
    guideContainerView.layer.borderColor = UIColor.blackDividers()?.cgColor
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
    moreButton.isHidden = true
    moreButtonHeightConstraint.constant = 0
    placeDescriptionLabel.numberOfLines = 0
    onMore?()
  }

  @IBAction func onCatalogButton(_ sender: UIButton) {
    onGoToCatalog?()
  }
}
