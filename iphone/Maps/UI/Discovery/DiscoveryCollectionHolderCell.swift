class DiscoveryCollectionHolder: UITableViewCell {
  @IBOutlet private(set) weak var collectionView: UICollectionView!
  @IBOutlet private weak var header: UILabel!

  fileprivate func config(header: String, cellClass: AnyClass) {
    collectionView.register(cellClass: cellClass)
    self.header.text = header
  }
}

@objc(MWMDiscoveryViatorCollectionHolderCell)
final class DiscoveryViatorCollectionHolderCell: DiscoveryCollectionHolder {
  typealias Tap = () -> ()
  private var tap: Tap?

  @objc func config(tap: @escaping Tap) {
    self.tap = tap
    super.config(header: L("discovery_button_subtitle_things_to_do").uppercased(),
                 cellClass: ViatorElement.self)
  }

  @IBAction private func onTap() {
    tap?()
  }
}

@objc(MWMDiscoveryLocalExpertCollectionHolderCell)
final class DiscoveryLocalExpertCollectionHolderCell: DiscoveryCollectionHolder {
  @objc func config() {
    super.config(header: L("discovery_button_subtitle_local_guides").uppercased(),
                 cellClass: DiscoveryLocalExpertCell.self)
  }
}

@objc(MWMDiscoverySearchCollectionHolderCell)
final class DiscoverySearchCollectionHolderCell: DiscoveryCollectionHolder {
  @objc func configAttractionsCell() {
    config(header: L("discovery_button_subtitle_attractions").uppercased())
  }

  @objc func configCafesCell() {
    config(header: L("discovery_button_subtitle_eat_and_drink").uppercased())
  }

  private func config(header: String) {
    super.config(header: header, cellClass: DiscoverySearchCell.self)
  }
}
