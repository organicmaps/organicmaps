/// Computes the geometry of the bottom menu sheet independently of UIKit
/// constraints, so the sizing logic is a single, testable seam.
enum BottomMenuSheetLayout {
  /// Desired presentation height of the sheet, capped to the available space.
  /// Returns `nil` when there is nothing to present yet (e.g. not laid out).
  static func height(contentHeight: CGFloat, maximumHeight: CGFloat) -> CGFloat? {
    guard contentHeight > 0, maximumHeight > 0 else { return nil }
    return min(contentHeight, maximumHeight)
  }

  /// Whether the content overflows the sheet and therefore must scroll.
  static func isScrollable(contentHeight: CGFloat, sheetHeight: CGFloat) -> Bool {
    contentHeight > sheetHeight
  }
}
