final class TrackStatisticsTableViewCell: UITableViewCell {

  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: .value1, reuseIdentifier: reuseIdentifier)
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  func configure(text: String, detailText: String) {
    let detailTextFont: UIFont = .bold16()
    let detailTextColor: UIColor = .black
    if #available(iOS 14.0, *) {
      var configuration = UIListContentConfiguration.valueCell()
      configuration.text = text
      configuration.secondaryText = detailText
      configuration.secondaryTextProperties.font = detailTextFont
      configuration.secondaryTextProperties.color = detailTextColor
      contentConfiguration = configuration
    } else {
      textLabel?.text = text
      detailTextLabel?.text = detailText
      detailTextLabel?.font = detailTextFont
      detailTextLabel?.textColor = detailTextColor
    }
  }
}
