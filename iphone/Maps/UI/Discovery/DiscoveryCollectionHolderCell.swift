class DiscoveryCollectionHolder: UITableViewCell {
  @IBOutlet private(set) weak var collectionView: UICollectionView!
  @IBOutlet fileprivate weak var header: UILabel!
}

@objc(MWMDiscoveryViatorCollectionHolderCell)
final class DiscoveryViatorCollectionHolderCell: DiscoveryCollectionHolder {
  typealias Tap = () -> Void
  private var tap: Tap?

  @objc func config(tap: @escaping Tap) {
    self.tap = tap
    header.text = L("discovery_button_subtitle_things_to_do").uppercased()
    collectionView.register(cellClass: ViatorElement.self)
  }

  @IBAction private func onTap() {
    tap?()
  }
}

@objc(MWMDiscoveryLocalExpertCollectionHolderCell)
final class DiscoveryLocalExpertCollectionHolderCell: DiscoveryCollectionHolder {
  @objc func config() {
    header.text = L("discovery_button_subtitle_local_guides").uppercased()
    collectionView.register(cellClass: DiscoveryLocalExpertCell.self)
    collectionView.register(cellClass: DiscoveryMoreCell.self)
  }
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
