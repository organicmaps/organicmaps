enum CornerRadius {
  case modalSheet
  case buttonDefault
  case buttonDefaultSmall
  case buttonSmall
  case buttonDefaultBig
  case grabber
  case custom(CGFloat)
}

extension CornerRadius {
  var value: CGFloat {
    switch self {
    case .modalSheet: return 12
    case .buttonDefault: return 8
    case .buttonDefaultSmall: return 6
    case .buttonSmall: return 4
    case .buttonDefaultBig: return 12
    case .grabber: return 2.5
    case .custom(let value): return value
    }
  }
}
