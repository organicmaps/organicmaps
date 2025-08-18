import UIKit

final class SelectedLastCell: SelectedSingleCell {
  let rangeLeadingView = UIView()

  override func addSubviews() {
    contentView.addSubview(rangeLeadingView)
    rangeLeadingView.translatesAutoresizingMaskIntoConstraints = false
    NSLayoutConstraint.activate([
      rangeLeadingView.leadingAnchor.constraint(equalTo: contentView.leadingAnchor),
      rangeLeadingView.topAnchor.constraint(equalTo: contentView.topAnchor, constant: 4),
      rangeLeadingView.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: -4),
      rangeLeadingView.trailingAnchor.constraint(equalTo: contentView.centerXAnchor)
    ])
    super.addSubviews()
  }

  override func updateTheme() {
    super.updateTheme()
    rangeLeadingView.backgroundColor = theme.selectedRangeBackgroundColor
  }
}
