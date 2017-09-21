@objc(MWMFilterCollectionHolderCell)
final class FilterCollectionHolderCell: MWMTableViewCell {
  @IBOutlet weak var collectionView: UICollectionView!
  @IBOutlet private weak var collectionViewHeight: NSLayoutConstraint!
  private weak var tableView: UITableView?
  override var frame: CGRect {
    didSet {
      guard #available(iOS 10, *) else {
        if frame.size.height < 1 /* minimal correct height */ {
          frame.size.height = max(collectionViewHeight.constant, 1)
          tableView?.refresh()
        }
        return
      }
    }
  }

  private func layout() {
    collectionView.setNeedsLayout()
    collectionView.layoutIfNeeded()
    collectionViewHeight.constant = collectionView.contentSize.height
  }

  @objc func config(tableView: UITableView?) {
    layout()
    collectionView.allowsMultipleSelection = true
    isSeparatorHidden = true
    backgroundColor = UIColor.pressBackground()

    guard #available(iOS 10, *) else {
      self.tableView = tableView
      return
    }
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    layout()
  }
}
