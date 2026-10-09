final class MapPointPickerDoneButton: UIButton {
  override var accessibilityValue: String? {
    get {
      guard !FrameworkHelper.isFrameworkDestroyed() else { return nil }
      let center = FrameworkHelper.mercatorViewportCenter()
      return FrameworkHelper.address(at: center) ?? FrameworkHelper.formattedCoordinates(at: center)
    }
    set { super.accessibilityValue = newValue }
  }
}
