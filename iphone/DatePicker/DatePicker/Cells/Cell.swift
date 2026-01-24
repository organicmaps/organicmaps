import UIKit

class Cell: UICollectionViewCell {
  let label = UILabel()

  var theme = DatePickerViewTheme() {
    didSet {
      updateTheme()
    }
  }

  func updateTheme() {
    label.textColor = theme.dayColor
    label.font = theme.dayFont
  }

  override init(frame: CGRect) {
    super.init(frame: frame)
    addSubviews()
    label.textAlignment = .center
    updateTheme()
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  func addSubviews() {
    contentView.addSubview(label)
    label.alignToSuperview()
  }

  override func prepareForReuse() {
    label.text = nil
  }
}
