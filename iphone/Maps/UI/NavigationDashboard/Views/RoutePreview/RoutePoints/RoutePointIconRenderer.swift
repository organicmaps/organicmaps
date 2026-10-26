import UIKit

enum RoutePointIconRenderer {
  private static let cache: NSCache<NSNumber, UIImage> = {
    let cache = NSCache<NSNumber, UIImage>()
    cache.countLimit = 100
    return cache
  }()

  static func image(number: Int) -> UIImage {
    precondition(number > 0)
    let key = NSNumber(value: number)
    if let image = cache.object(forKey: key) {
      return image
    }

    let background = UIImage(resource: .routePointBackground)
    let text = String(number) as NSString
    let textInset: CGFloat = 3.5
    let availableWidth = background.size.width - 2 * textInset
    let font = fittedFont(for: text, maxWidth: availableWidth)
    let attributes: [NSAttributedString.Key: Any] = [.font: font, .foregroundColor: UIColor.white]
    let textSize = text.size(withAttributes: attributes)
    let image = UIGraphicsImageRenderer(size: background.size).image { _ in
      background.draw(at: .zero)
      text.draw(at: CGPoint(x: (background.size.width - textSize.width) / 2,
                            y: (background.size.height - textSize.height) / 2), withAttributes: attributes)
    }.withRenderingMode(.alwaysOriginal)
    // Fixed badge colors keep cached images valid across appearance changes.
    cache.setObject(image, forKey: key)
    return image
  }

  static func fittedFont(for text: NSString, maxWidth: CGFloat) -> UIFont {
    let baseFont = UIFont.bold14
    let naturalWidth = text.size(withAttributes: [.font: baseFont]).width
    var font = baseFont.withSize(baseFont.pointSize * min(1, maxWidth / naturalWidth))
    // Font metrics do not scale exactly with the point size.
    while text.size(withAttributes: [.font: font]).width > maxWidth {
      font = font.withSize(font.pointSize - 0.5)
    }
    return font
  }
}
