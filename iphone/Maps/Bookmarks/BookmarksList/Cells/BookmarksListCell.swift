final class BookmarksListCell: UITableViewCell {

  private static let extendedImageViewTappableMargin: CGFloat = -15

  private var trackColorDidTapAction: (() -> Void)?

  override func awakeFromNib() {
    super.awakeFromNib()
    setupCell()
  }

  private func setupCell() {
    accessoryType = .detailButton
    if let imageView {
      let tapGesture = UITapGestureRecognizer(target: self, action: #selector(colorDidTapAction(_:)))
      imageView.addGestureRecognizer(tapGesture)
      imageView.isUserInteractionEnabled = true
    }
  }

  func config(_ bookmark: IBookmarksListItemViewModel) {
    imageView?.image = bookmark.image
    textLabel?.text = bookmark.name
    detailTextLabel?.text = bookmark.subtitle
    trackColorDidTapAction = bookmark.colorDidTapAction
  }

  @objc private func colorDidTapAction(_ sender: UITapGestureRecognizer) {
    trackColorDidTapAction?()
  }

  // Extends the imageView tappable area.
  override func hitTest(_ point: CGPoint, with event: UIEvent?) -> UIView? {
    if let imageView, imageView.convert(imageView.bounds, to: self).insetBy(dx: Self.extendedImageViewTappableMargin, dy: Self.extendedImageViewTappableMargin).contains(point) {
      return imageView
    } else {
      return super.hitTest(point, with: event)
    }
  }
}
