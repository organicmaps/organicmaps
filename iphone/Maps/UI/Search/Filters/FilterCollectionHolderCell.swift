@objc(MWMFilterCollectionHolderCell)
final class FilterCollectionHolderCell: MWMTableViewCell {
  @IBOutlet weak var collectionView: UICollectionView!
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
    collectionViewHeight.constant = collectionView.contentSize.height
  }

  @objc func config(tableView: UITableView?) {
    self.tableView = tableView
    layout()
    collectionView.allowsMultipleSelection = true
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
