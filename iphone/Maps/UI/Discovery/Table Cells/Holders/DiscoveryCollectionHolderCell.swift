class DiscoveryCollectionHolder: UITableViewCell {
  @IBOutlet private(set) weak var collectionView: UICollectionView!
  @IBOutlet fileprivate weak var header: UILabel!
}

@objc(MWMDiscoverySearchCollectionHolderCell)
final class DiscoverySearchCollectionHolderCell: DiscoveryCollectionHolder {
  @objc func configAttractionsCell() {
    config(text: L("discovery_button_subtitle_attractions").uppercased())
  }

  @objc func configCafesCell() {
    config(text: L("discovery_button_subtitle_eat_and_drink").uppercased())
  }

  private func config(text: String) {
    header.text = text
    collectionView.register(cellClass: DiscoverySearchCell.self)
    collectionView.register(cellClass: DiscoveryMoreCell.self)
  }
}

@objc(MWMDiscoveryBookingCollectionHolderCell)
final class DiscoveryBookingCollectionHolderCell: DiscoveryCollectionHolder {
  @objc func config() {
    header.text = L("discovery_button_subtitle_book_hotels").uppercased()
    collectionView.register(cellClass: DiscoveryBookingCell.self)
    collectionView.register(cellClass: DiscoveryMoreCell.self)
  }
}

@objc(MWMDiscoveryGuideCollectionHolderCell)
final class DiscoveryGuideCollectionHolderCell: DiscoveryCollectionHolder {
  @objc func config(_ title: String) {
    header.text = title.uppercased()
    collectionView.register(cellClass: DiscoveryGuideCell.self)
    collectionView.register(cellClass: DiscoveryMoreCell.self)
  }
}
