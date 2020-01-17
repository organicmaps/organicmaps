@objc(MWMUGCSummaryRatingCell)
final class UGCSummaryRatingCell: MWMTableViewCell {
  @IBOutlet private weak var titleLabel: UILabel!
  @IBOutlet private weak var countLabel: UILabel!
  @IBOutlet private weak var ratingSummaryView: RatingSummaryView!
  @IBOutlet private weak var ratingCollectionViewHeight: NSLayoutConstraint!
  @IBOutlet private weak var ratingCollectionView: UICollectionView! {
    didSet {
      ratingCollectionView.register(cellClass: UGCSummaryRatingStarsCell.self)
    }
  }

  fileprivate var ratings: [UGCRatingStars]!

  override var frame: CGRect {
    didSet {
      if frame.size != oldValue.size {
        updateCollectionView()
      }
    }
  }

  @objc func config(reviewsCount: UInt, summaryRating: UGCRatingValueType, ratings: [UGCRatingStars]) {
    countLabel.text = String(format:L("placepage_summary_rating_description"), reviewsCount)
    ratingSummaryView.value = summaryRating.value
    ratingSummaryView.type = summaryRating.type
    self.ratings = ratings
    updateCollectionView()
    isSeparatorHidden = true
  }

  override func didMoveToSuperview() {
    super.didMoveToSuperview()
    updateCollectionView()
  }

  private func updateCollectionView() {
    DispatchQueue.main.async {
      self.ratingCollectionView.reloadData()
    }
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
