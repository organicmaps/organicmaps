@objc(MWMFilterCollectionHolderCell)
final class FilterCollectionHolderCell: MWMTableViewCell {
  @IBOutlet private(set) weak var collectionView: UICollectionView!
  @IBOutlet private weak var collectionViewHeight: NSLayoutConstraint!
  private weak var tableView: UITableView?
  override var frame: CGRect {
    didSet {
      if frame.size.height < 1 /* minimal correct height */ {
        frame.size.height = max(collectionViewHeight.constant, 1)
        tableView?.refresh()
      }
    }
  }

  private func layout() {
    collectionView.setNeedsLayout()
    collectionView.layoutIfNeeded()
    if collectionViewHeight.constant != collectionView.contentSize.height {
      collectionViewHeight.constant = collectionView.contentSize.height
      frame.size.height = collectionViewHeight.constant
    }
  }

  @objc func config(tableView: UITableView?) {
    self.tableView = tableView
    layout()
    collectionView.allowsMultipleSelection = true
    collectionView.contentInset = UIEdgeInsets(top: 0, left: 16, bottom: 0, right: 16)
    collectionView.reloadData()
  }

  override func awakeFromNib() {
    super.awakeFromNib()
    isSeparatorHidden = true
    backgroundColor = UIColor.clear
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    layout()
  }
}
