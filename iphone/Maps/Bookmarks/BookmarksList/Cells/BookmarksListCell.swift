final class BookmarksListCell: UITableViewCell {

  func config(_ bookmark: IBookmarksListItemViewModel) {
    imageView?.image = bookmark.image
    textLabel?.text = bookmark.name
    detailTextLabel?.text = bookmark.subtitle
  }
}
