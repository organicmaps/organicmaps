protocol SettingsTextFieldCellDelegate: AnyObject {
  func textFieldCell(_ cell: SettingsTextFieldCell, didChangeText text: String)
  func textFieldCell(_ cell: SettingsTextFieldCell, didEndEditingText text: String)
}

final class SettingsTextFieldCell: MWMTableViewCell {
  private let textField = UITextField(frame: .zero)
  private weak var delegate: SettingsTextFieldCellDelegate?

  override init(style _: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: .default, reuseIdentifier: reuseIdentifier)
    setupCell()
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    super.init(coder: coder)
  }

  override func prepareForReuse() {
    super.prepareForReuse()
    delegate = nil
    textField.text = nil
    textField.placeholder = nil
    setValid(true)
  }

  func configure(delegate: SettingsTextFieldCellDelegate,
                 text: String,
                 placeholder: String?,
                 isEnabled: Bool,
                 isValid: Bool) {
    let wasFirstResponder = textField.isFirstResponder
    self.delegate = delegate
    if textField.text != text {
      textField.text = text
    }
    textField.placeholder = placeholder
    textField.isEnabled = isEnabled
    textField.textColor = isEnabled ? .label : .tertiaryLabel
    setValid(isValid)
    if wasFirstResponder, isEnabled {
      textField.becomeFirstResponder()
    }
  }

  private func setupCell() {
    setStyle(.background)
    selectionStyle = .none

    textField.translatesAutoresizingMaskIntoConstraints = false
    textField.delegate = self
    textField.returnKeyType = .done
    textField.clearButtonMode = .whileEditing
    textField.keyboardType = .default
    textField.autocapitalizationType = .none
    textField.autocorrectionType = .no
    textField.addTarget(self, action: #selector(textFieldChanged), for: .editingChanged)

    contentView.addSubview(textField)
    let margins = contentView.layoutMarginsGuide
    NSLayoutConstraint.activate([
      textField.leadingAnchor.constraint(equalTo: margins.leadingAnchor),
      textField.trailingAnchor.constraint(equalTo: margins.trailingAnchor),
      textField.topAnchor.constraint(equalTo: margins.topAnchor),
      textField.bottomAnchor.constraint(equalTo: margins.bottomAnchor),
    ])
  }

  private func setValid(_ isValid: Bool) {
    contentView.setStyleAndApply(isValid ? .background : .errorBackground)
  }
}

extension SettingsTextFieldCell: UITextFieldDelegate {
  @objc private func textFieldChanged(_ textField: UITextField) {
    delegate?.textFieldCell(self, didChangeText: textField.text ?? "")
  }

  func textFieldShouldReturn(_ textField: UITextField) -> Bool {
    textField.resignFirstResponder()
    return true
  }

  func textFieldDidEndEditing(_ textField: UITextField) {
    delegate?.textFieldCell(self, didEndEditingText: textField.text ?? "")
  }
}
