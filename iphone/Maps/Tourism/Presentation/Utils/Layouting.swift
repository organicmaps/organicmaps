func applyWrapContent(label: UILabel) {
  label.numberOfLines = 0
  label.lineBreakMode = .byWordWrapping
  label.setContentCompressionResistancePriority(.required, for: .vertical)
  label.setContentHuggingPriority(.defaultLow, for: .vertical)
}
