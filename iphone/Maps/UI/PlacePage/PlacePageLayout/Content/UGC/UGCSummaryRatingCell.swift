@objc(MWMUGCSummaryRatingCell)
final class UGCSummaryRatingCell: MWMTableViewCell {
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
    countLabel.text = String(coreFormat: L("placepage_summary_rating_description"), arguments: [reviewsCount])
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
