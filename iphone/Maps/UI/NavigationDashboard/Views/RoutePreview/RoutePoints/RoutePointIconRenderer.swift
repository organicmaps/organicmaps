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
    let baseFont = UIFont.bold12
    let textInset: CGFloat = 3
    let availableWidth = background.size.width - 2 * textInset
    let naturalWidth = text.size(withAttributes: [.font: baseFont]).width
    let font = baseFont.withSize(baseFont.pointSize * min(1, availableWidth / naturalWidth))
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
}
