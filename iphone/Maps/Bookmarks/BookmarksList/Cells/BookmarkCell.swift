final class BookmarkCell: UITableViewCell {
  @IBOutlet private var bookmarkImageView: UIImageView!
  @IBOutlet private var bookmarkTitleLabel: UILabel!
  @IBOutlet private var bookmarkSubtitleLabel: UILabel!

  func config(_ bookmark: IBookmarkViewModel) {
    bookmarkImageView.image = bookmark.image
    bookmarkTitleLabel.text = bookmark.bookmarkName
    bookmarkSubtitleLabel.text = bookmark.subtitle
  }
}
