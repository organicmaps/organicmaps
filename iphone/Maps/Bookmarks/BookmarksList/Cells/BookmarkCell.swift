final class BookmarkCell: UITableViewCell {
  @IBOutlet private var bookmarkImageView: UIImageView!
  @IBOutlet private var bookmarkTitleLabel: UILabel!
  @IBOutlet private var bookmarkSubtitleLabel: UILabel!

  private var bookmarkColorDidTapAction: (() -> Void)?

  override func awakeFromNib() {
    super.awakeFromNib()
    let tapGesture = UITapGestureRecognizer(target: self, action: #selector(colorDidTapAction(_:)))
    bookmarkImageView.addGestureRecognizer(tapGesture)
  }

  func config(_ bookmark: IBookmarkViewModel) {
    bookmarkImageView.image = bookmark.image
    bookmarkTitleLabel.text = bookmark.bookmarkName
    bookmarkSubtitleLabel.text = bookmark.subtitle
    bookmarkColorDidTapAction = bookmark.colorDidTapAction
  }

  @objc private func colorDidTapAction(_ sender: UITapGestureRecognizer) {
    bookmarkColorDidTapAction?()
  }
}
