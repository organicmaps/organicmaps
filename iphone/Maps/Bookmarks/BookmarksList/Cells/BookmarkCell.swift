final class BookmarkCell: UITableViewCell {
  @IBOutlet var bookmarkImageView: UIImageView!
  @IBOutlet var bookmarkTitleLabel: UILabel!
  @IBOutlet var bookmarkSubtitleLabel: UILabel!

  func config(_ bookmark: IBookmarkViewModel) {
    bookmarkImageView.image = bookmark.image
    bookmarkTitleLabel.text = bookmark.bookmarkName
    bookmarkSubtitleLabel.text = bookmark.subtitle
  }
}
