import UIKit

final class SelectedFirstCell: SelectedSingleCell {
  let rangeTrailingView = UIView()

  override func addSubviews() {
    contentView.addSubview(rangeTrailingView)
    rangeTrailingView.translatesAutoresizingMaskIntoConstraints = false
    NSLayoutConstraint.activate([
      rangeTrailingView.leadingAnchor.constraint(equalTo: contentView.centerXAnchor),
      rangeTrailingView.topAnchor.constraint(equalTo: contentView.topAnchor, constant: 4),
      rangeTrailingView.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: -4),
      rangeTrailingView.trailingAnchor.constraint(equalTo: contentView.trailingAnchor)
    ])
    super.addSubviews()
  }

  override func updateTheme() {
    super.updateTheme()
    rangeTrailingView.backgroundColor = theme.selectedRangeBackgroundColor
  }
}
