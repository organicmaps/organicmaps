final class RecentlyDeletedTableViewCell: UITableViewCell {

  private static let dateFormatter: DateFormatter = {
    let formatter = DateFormatter()
    formatter.dateStyle = .medium
    formatter.timeStyle = .medium
    return formatter
  }()

  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: .subtitle, reuseIdentifier: reuseIdentifier)
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  func configureWith(_ category: RecentlyDeletedCategory) {
    textLabel?.text = category.fileName

    let date = Date(timeIntervalSince1970: category.deletionDate)
    detailTextLabel?.text = Self.dateFormatter.string(from: date)
  }
}
