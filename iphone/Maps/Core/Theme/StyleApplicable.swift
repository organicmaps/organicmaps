protocol StyleApplicable: AnyObject {
  var styleName: String { get set }
  var isStyleApplied: Bool { get set }

  func setStyleName(_ styleName: String)
  func setStyleNameAndApply(_ styleName: String)
  func setStyle(_ style: StyleStringRepresentable)
  func setStyleAndApply(_ style: StyleStringRepresentable)

  func applyTheme()
}

extension StyleApplicable {
  func setStyleName(_ styleName: String) {
    self.styleName = styleName
  }

  func setStyleNameAndApply(_ styleName: String) {
    self.styleName = styleName
    applyTheme()
  }

  func setStyle(_ style: StyleStringRepresentable) {
    styleName = style.rawValue
  }

  func setStyleAndApply(_ style: StyleStringRepresentable) {
    styleName = style.rawValue
    applyTheme()
  }
}

// Overload for the direct usage of the nested StyleSheet enums
extension StyleApplicable {
  func setStyle(_ style: GlobalStyleSheet) { setStyle(style) }
  func setStyle(_ style: PlacePageStyleSheet) { setStyle(style) }
  func setStyle(_ style: MapStyleSheet) { setStyle(style) }
  func setStyle(_ style: BookmarksStyleSheet) { setStyle(style) }
  func setStyle(_ style: SearchStyleSheet) { setStyle(style) }

  func setStyleAndApply(_ style: GlobalStyleSheet) { setStyleAndApply(style) }
  func setStyleAndApply(_ style: PlacePageStyleSheet) { setStyleAndApply(style) }
  func setStyleAndApply(_ style: MapStyleSheet) { setStyleAndApply(style) }
  func setStyleAndApply(_ style: BookmarksStyleSheet) { setStyleAndApply(style) }
  func setStyleAndApply(_ style: SearchStyleSheet) { setStyleAndApply(style) }

  func setStyles(_ styles: [GlobalStyleSheet]) { styleName = styles.joinedStyle }
}

private extension Collection where Element: RawRepresentable, Element.RawValue == String {
  var joinedStyle: String {
    map(\.rawValue).joined(separator: ":")
  }
}
