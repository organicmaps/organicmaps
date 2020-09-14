import Foundation

extension BookmarkColor {
  var title: String {
    localizedTitleForBookmarkColor(self)
  }

  var color: UIColor {
    uiColorForBookmarkColor(self)
  }

  func image(_ selected: Bool) -> UIImage {
    if selected {
      return circleImageForColor(color, frameSize: 22, iconName: "ic_bm_none")
    } else {
      return circleImageForColor(color, frameSize: 22, diameter: 14)
    }
  }

  func image(_ iconName: String) -> UIImage {
    circleImageForColor(color, frameSize: 22, iconName: iconName)
  }
}

fileprivate func titleForBookmarkColor(_ color: BookmarkColor) -> String {
  switch color {
  case .red: return "red"
  case .blue: return "blue"
  case .purple: return "purple"
  case .yellow: return "yellow"
  case .pink: return "pink"
  case .brown: return "brown"
  case .green: return "green"
  case .orange: return "orange"
  case .deepPurple: return "deep_purple"
  case .lightBlue: return "light_blue"
  case .cyan: return "cyan"
  case .teal: return "teal"
  case .lime: return "lime"
  case .deepOrange: return "deep_orange"
  case .gray: return "gray"
  case .blueGray: return "blue_gray"
  case .none, .count: return ""
  @unknown default:
    fatalError()
  }
}

fileprivate func localizedTitleForBookmarkColor(_ color: BookmarkColor) -> String {
  L(titleForBookmarkColor(color))
}

fileprivate func rgbColor(_ r: CGFloat, _ g: CGFloat, _ b: CGFloat) -> UIColor {
  UIColor(red: r / 255, green: g / 255, blue: b / 255, alpha: 0.8)
}

fileprivate func uiColorForBookmarkColor(_ color: BookmarkColor) -> UIColor {
  switch color {
  case .red: return rgbColor(229, 27, 35);
  case .pink: return rgbColor(255, 65, 130);
  case .purple: return rgbColor(155, 36, 178);
  case .deepPurple: return rgbColor(102, 57, 191);
  case .blue: return rgbColor(0, 102, 204);
  case .lightBlue: return rgbColor(36, 156, 242);
  case .cyan: return rgbColor(20, 190, 205);
  case .teal: return rgbColor(0, 165, 140);
  case .green: return rgbColor(60, 140, 60);
  case .lime: return rgbColor(147, 191, 57);
  case .yellow: return rgbColor(255, 200, 0);
  case .orange: return rgbColor(255, 150, 0);
  case .deepOrange: return rgbColor(240, 100, 50);
  case .brown: return rgbColor(128, 70, 51);
  case .gray: return rgbColor(115, 115, 115);
  case .blueGray: return rgbColor(89, 115, 128);
  case .none, .count:
    fatalError()
  @unknown default:
    fatalError()
  }
}

func circleImageForColor(_ color: UIColor,
                         frameSize: CGFloat,
                         diameter: CGFloat? = nil,
                         iconName: String? = nil) -> UIImage {
  let renderer = UIGraphicsImageRenderer(size: CGSize(width: frameSize, height: frameSize))
  return renderer.image { context in
    let d = diameter ?? frameSize
    let rect = CGRect(x: (frameSize - d) / 2, y: (frameSize - d) / 2, width: d, height: d)
    context.cgContext.addEllipse(in: rect)
    context.cgContext.setFillColor(color.cgColor)
    context.cgContext.fillPath()

    guard let iconName = iconName, let image = UIImage(named: iconName) else { return }
    image.draw(in: rect.insetBy(dx: 3, dy: 3))
  }
}
