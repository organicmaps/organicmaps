@objc
final class SettingsTableViewLinkCell: MWMTableViewCell {
  override init(style _: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: .value1, reuseIdentifier: reuseIdentifier)
    setupCell()
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    super.init(coder: coder)
  }

  override func awakeFromNib() {
    super.awakeFromNib()
    setupCell()
  }

  private func setupCell() {
    setStyle(.background)
    textLabel?.setFontStyle(.regular17, color: .blackPrimary)
    textLabel?.numberOfLines = 0
    textLabel?.lineBreakMode = .byWordWrapping
    detailTextLabel?.setFontStyle(.regular17, color: .blackSecondary)
    accessoryType = .disclosureIndicator
    selectionStyle = .default
  }

  @objc func config(title: String, info: String?) {
    textLabel?.text = title
    detailTextLabel?.text = info
    detailTextLabel?.isHidden = info == nil
  }
}
