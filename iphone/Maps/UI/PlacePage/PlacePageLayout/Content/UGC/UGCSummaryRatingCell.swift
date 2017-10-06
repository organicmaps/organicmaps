@objc(MWMUGCSummaryRatingCell)
final class UGCSummaryRatingCell: MWMTableViewCell {
  private enum Config {
    static let minimumInteritemSpacing: CGFloat = 16
    static let minItemsPerRow: CGFloat = 3
    static let estimatedItemSize = CGSize(width: 96, height: 32)
  }

  @IBOutlet private weak var titleLabel: UILabel! {
    didSet {
      titleLabel.font = UIFont.bold22()
      titleLabel.textColor = UIColor.blackPrimaryText()
      titleLabel.text = L("placepage_summary_rating")
    }
  }

  @IBOutlet private weak var countLabel: UILabel! {
    didSet {
      countLabel.font = UIFont.regular12()
      countLabel.textColor = UIColor.blackSecondaryText()
    }
  }

  @IBOutlet private weak var ratingSummaryView: RatingSummaryView! {
    didSet {
      ratingSummaryView.horribleColor = UIColor.ratingRed()
      ratingSummaryView.badColor = UIColor.ratingOrange()
      ratingSummaryView.normalColor = UIColor.ratingYellow()
      ratingSummaryView.goodColor = UIColor.ratingLightGreen()
      ratingSummaryView.excellentColor = UIColor.ratingGreen()
      ratingSummaryView.textFont = UIFont.bold28()
      ratingSummaryView.textSize = 28
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

  fileprivate var ratings: [UGCRatingStars]!

  override var frame: CGRect {
    didSet {
      if frame.size != oldValue.size {
        updateCollectionView(nil)
      }
    }
  }

  @objc func config(reviewsCount: UInt, summaryRating: UGCRatingValueType, ratings: [UGCRatingStars]) {
    countLabel.text = String(coreFormat: L("place_page_summary_rating_description"), arguments: [reviewsCount])
    ratingSummaryView.value = summaryRating.value
    ratingSummaryView.type = summaryRating.type
    self.ratings = ratings
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
    let layout = ratingCollectionView.collectionViewLayout as! UICollectionViewFlowLayout
    let inset = layout.sectionInset
    let viewWidth = sv.size.width - inset.left - inset.right
    let maxItemWidth = layout.estimatedItemSize.width

    let ratingsCount = CGFloat(ratings?.count ?? 0)
    let itemsPerRow = floor(min(max(viewWidth / maxItemWidth, Config.minItemsPerRow), ratingsCount))
    let itemWidth = floor(min((viewWidth - (itemsPerRow - 1) * Config.minimumInteritemSpacing) / itemsPerRow, maxItemWidth))
    let interitemSpacing = floor((viewWidth - itemWidth * itemsPerRow) / (itemsPerRow - 1))
    layout.minimumInteritemSpacing = interitemSpacing
    layout.itemSize = CGSize(width: itemWidth, height: Config.estimatedItemSize.height)

    let rowsCount = ceil(ratingsCount / itemsPerRow)
    ratingCollectionViewHeight.constant = rowsCount * Config.estimatedItemSize.height + (rowsCount - 1) * layout.minimumLineSpacing + inset.top + inset.bottom
    ratingCollectionView.performBatchUpdates(updates, completion: nil)
  }
}

extension UGCSummaryRatingCell: UICollectionViewDataSource {
  func collectionView(_: UICollectionView, numberOfItemsInSection _: Int) -> Int {
    return ratings?.count ?? 0
  }

  func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
    let cell = collectionView.dequeueReusableCell(withCellClass: UGCSummaryRatingStarsCell.self, indexPath: indexPath) as! UGCSummaryRatingStarsCell
    cell.config(rating: ratings[indexPath.item])
    return cell
  }
}
