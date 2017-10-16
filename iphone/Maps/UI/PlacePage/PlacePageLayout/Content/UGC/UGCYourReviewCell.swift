@objc(MWMUGCYourReviewCell)
final class UGCYourReviewCell: MWMTableViewCell {
  private enum Config {
    static let minimumInteritemSpacing: CGFloat = 16
    static let minItemsPerRow: CGFloat = 3
    static let estimatedItemSize = CGSize(width: 96, height: 32)
  }

  @IBOutlet private weak var titleLabel: UILabel! {
    didSet {
      titleLabel.font = UIFont.bold14()
      titleLabel.textColor = UIColor.blackPrimaryText()
      titleLabel.text = L("placepage_reviews_your_comment")
    }
  }

  @IBOutlet private weak var dateLabel: UILabel! {
    didSet {
      dateLabel.font = UIFont.regular12()
      dateLabel.textColor = UIColor.blackSecondaryText()
    }
  }

  @IBOutlet private weak var reviewLabel: ExpandableTextView! {
    didSet {
      reviewLabel.textFont = UIFont.regular14()
      reviewLabel.textColor = UIColor.blackPrimaryText()
      reviewLabel.expandText = L("placepage_more_button")
      reviewLabel.expandTextColor = UIColor.linkBlue()
    }
  }

  @IBOutlet private weak var ratingCollectionViewHeight: NSLayoutConstraint!
  @IBOutlet private weak var ratingCollectionView: UICollectionView! {
    didSet {
      ratingCollectionView.register(cellClass: UGCSummaryRatingStarsCell.self)
      let layout = ratingCollectionView.collectionViewLayout as! UICollectionViewFlowLayout
      layout.estimatedItemSize = Config.estimatedItemSize
    }
  }

  private var yourReview: UGCYourReview!

  override var frame: CGRect {
    didSet {
      if frame.size != oldValue.size {
        updateCollectionView(nil)
      }
    }
  }

  @objc func config(yourReview: UGCYourReview, onUpdate: @escaping () -> Void) {
    let dateFormatter = DateFormatter()
    dateFormatter.dateStyle = .medium
    dateFormatter.timeStyle = .none
    dateLabel.text = dateFormatter.string(from: yourReview.date)
    self.yourReview = yourReview
    reviewLabel.text = yourReview.text
    reviewLabel.onUpdate = onUpdate
    updateCollectionView { [weak self] in
      self?.ratingCollectionView.reloadSections(IndexSet(integer: 0))
    }
    isSeparatorHidden = true
  }

  override func didMoveToSuperview() {
    super.didMoveToSuperview()
    updateCollectionView(nil)
  }

  private func updateCollectionView(_ updates: (() -> Void)?) {
    guard let sv = superview else { return }
    DispatchQueue.main.async {
      let layout = self.ratingCollectionView.collectionViewLayout as! UICollectionViewFlowLayout
      let inset = layout.sectionInset
      let viewWidth = sv.size.width - inset.left - inset.right
      let maxItemWidth = layout.estimatedItemSize.width

      let ratingsCount = CGFloat(self.yourReview?.ratings.count ?? 0)
      let itemsPerRow = floor(min(max(viewWidth / maxItemWidth, Config.minItemsPerRow), ratingsCount))
      let itemWidth = floor(min((viewWidth - (itemsPerRow - 1) * Config.minimumInteritemSpacing) / itemsPerRow, maxItemWidth))
      let interitemSpacing = floor((viewWidth - itemWidth * itemsPerRow) / (itemsPerRow - 1))
      layout.minimumInteritemSpacing = interitemSpacing
      layout.itemSize = CGSize(width: itemWidth, height: Config.estimatedItemSize.height)

      assert(itemsPerRow > 0);
      let rowsCount = ceil(ratingsCount / itemsPerRow)
      self.ratingCollectionViewHeight.constant = rowsCount * Config.estimatedItemSize.height + (rowsCount - 1) * layout.minimumLineSpacing + inset.top + inset.bottom
      self.ratingCollectionView.performBatchUpdates(updates, completion: nil)
    }
  }
}

extension UGCYourReviewCell: UICollectionViewDataSource {
  func collectionView(_: UICollectionView, numberOfItemsInSection _: Int) -> Int {
    return yourReview?.ratings.count ?? 0
  }

  func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
    let cell = collectionView.dequeueReusableCell(withCellClass: UGCSummaryRatingStarsCell.self, indexPath: indexPath) as! UGCSummaryRatingStarsCell
    cell.config(rating: yourReview!.ratings[indexPath.item])
    return cell
  }
}
