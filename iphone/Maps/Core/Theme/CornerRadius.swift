enum CornerRadius {
  case modalSheet
  case buttonDefault
  case buttonDefaultSmall
  case buttonDefaultBig
  case buttonSmall
  case grabber
  case custom(CGFloat)
}

extension CornerRadius {
  var value: CGFloat {
    switch self {
    case .modalSheet:
      if #available(iOS 26.0, *) {
        // The iOS 26 uses 38 radiuses for the modal screen.
        return 28
      } else {
        return 12
      }
    case .buttonDefault: return 8
    case .buttonDefaultSmall: return 6
    case .buttonDefaultBig: return 12
    case .buttonSmall: return 4
    case .grabber: return 2.5
    case .custom(let value): return value
    }
  }
}
