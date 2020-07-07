class CatalogPromoItemCell: UICollectionViewCell {
  @IBOutlet var itemImageView: UIImageView!
  @IBOutlet var titleLabel: UILabel!
  @IBOutlet var subtitleLabel: UILabel!
  @IBOutlet var proLabel: UILabel!
  @IBOutlet var proContainerView: UIView!

  typealias OnDetails = () -> Void
  private var onDetails: OnDetails?

  override func prepareForReuse() {
    super.prepareForReuse()
    itemImageView.image = UIImage(named: "img_guide_placeholder")
    titleLabel.text = ""
    subtitleLabel.text = ""
    proLabel.text = ""
    proContainerView.isHidden = true
    onDetails = nil
  }

  func config(promoItem: CatalogPromoItem, onDetails: @escaping OnDetails) {
    setImage(promoItem.imageUrl)
    titleLabel.text = promoItem.guideName
    subtitleLabel.text = promoItem.guideAuthor
    self.onDetails = onDetails
    guard !promoItem.categoryLabel.isEmpty else {
      proContainerView.isHidden = true
      return
    }
    proLabel.text = promoItem.categoryLabel
    if promoItem.hexColor.count == 6 {
      proContainerView.backgroundColor = UIColor(fromHexString: promoItem.hexColor)
    } else {
      proContainerView.backgroundColor = UIColor.red
    }
    proContainerView.isHidden = false
  }

  @IBAction func onDetails(_ sender: UIButton) {
    onDetails?()
  }

  private func setImage(_ imageUrl: String?) {
    guard let imageUrl = imageUrl else { return }
    if let url = URL(string: imageUrl) {
      itemImageView.image = UIImage(named: "img_guide_placeholder")
      itemImageView.wi_setImage(with: url, transitionDuration: kDefaultAnimationDuration)
    } else {
      itemImageView.image = UIImage(named: "img_guide_placeholder")
    }
  }
}

protocol CatalogGalleryViewControllerDelegate: AnyObject {
  func promoGalleryDidSelectItemAtIndex(_ index: Int)
  func promoGalleryDidPressMore()
}

class CatalogGalleryViewController: UIViewController {
  @IBOutlet var titleLabel: UILabel!
  @IBOutlet var collectionView: UICollectionView!

  weak var delegate: CatalogGalleryViewControllerDelegate?

  var promoData: CatalogPromoData? {
    didSet {
      guard let promoData = promoData else { return }
      collectionView?.reloadData()
      titleLabel.text = String(coreFormat: L("pp_discovery_place_related_tag_header"),
                               arguments: [promoData.tagsString]).uppercased()
    }
  }
}

extension CatalogGalleryViewController: UICollectionViewDataSource {
  func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
    guard let promoData = promoData else { return 0 }
    return promoData.promoItems.count + 1
  }

  func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
    if indexPath.item < promoData!.promoItems.count {
      let cell = collectionView.dequeueReusableCell(withReuseIdentifier: "DiscoveryGuideCell", for: indexPath) as! CatalogPromoItemCell
      cell.config(promoItem: promoData!.promoItems[indexPath.item]) { [weak self] in
        self?.delegate?.promoGalleryDidSelectItemAtIndex(indexPath.item)
      }
      return cell
    } else {
      let cell = collectionView.dequeueReusableCell(withReuseIdentifier: "MoreCell", for: indexPath)
      return cell
    }
  }
}

extension CatalogGalleryViewController: UICollectionViewDelegate {
  func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
    if indexPath.item == promoData!.promoItems.count {
      delegate?.promoGalleryDidPressMore()
    } else {
      delegate?.promoGalleryDidSelectItemAtIndex(indexPath.item)
    }
  }
}
