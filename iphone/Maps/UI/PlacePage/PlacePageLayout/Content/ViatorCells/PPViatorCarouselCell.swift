@objc(MWMPPViatorCarouselCell)
final class PPViatorCarouselCell: MWMTableViewCell {
  @IBOutlet private weak var title: UILabel! {
    didSet {
      title.text = L("place_page_viator_title")
      title.font = UIFont.bold14()
      title.textColor = UIColor.blackSecondaryText()
    }
  }
  @IBOutlet private weak var more: UIButton! {
    didSet {
      more.setTitle(L("placepage_more_button"), for: .normal)
      more.titleLabel?.font = UIFont.regular17()
      more.setTitleColor(UIColor.linkBlue(), for: .normal)
    }
  }

  @IBOutlet private weak var collectionView: UICollectionView!
  fileprivate var dataSource: [ViatorItemModel] = []
  fileprivate let kMaximumNumberOfElements = 5
  fileprivate var delegate: MWMPlacePageButtonsProtocol?

  func config(with ds: [ViatorItemModel], delegate d: MWMPlacePageButtonsProtocol?) {
    if ds.isEmpty {
      Statistics.logEvent(kStatPlacepageSponsoredError)
    }
    dataSource = ds
    delegate = d
    collectionView.contentOffset = .zero
    collectionView.delegate = self
    collectionView.dataSource = self
    collectionView.register(cellClass: ViatorElement.self)
    collectionView.reloadData()

    isSeparatorHidden = true
    backgroundColor = UIColor.clear
  }

  fileprivate func isLastCell(_ indexPath: IndexPath) -> Bool {
    return indexPath.item == collectionView.numberOfItems(inSection: indexPath.section) - 1
  }

  @IBAction
  func onMore() {
    delegate?.openViatorURL(nil)
  }
}

extension PPViatorCarouselCell: UICollectionViewDelegate, UICollectionViewDataSource {
  func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
    let cell = collectionView.dequeueReusableCell(withCellClass: ViatorElement.self,
                                                  indexPath: indexPath) as! ViatorElement
    cell.model = isLastCell(indexPath) ? nil : dataSource[indexPath.item]
    cell.onMoreAction = { [weak self] in
      self?.onMore()
    }
    return cell
  }

  func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
    return min(dataSource.count + 1, kMaximumNumberOfElements)
  }

  func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
    let isMore = isLastCell(indexPath)
    let url: URL? = isMore ? nil : dataSource[indexPath.item].pageURL
    delegate?.openViatorURL(url)
    Statistics.logEvent(isMore ? kStatPlacepageSponsoredMoreSelected : kStatPlacepageSponsoredItemSelected)
  }
}
