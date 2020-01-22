
struct RatingViewSettings {
  enum FillMode: Int {
    case full = 0
    case half = 1
    case precise = 2
  }

  enum TextSide {
    case left
    case right
    case top
    case bottom
  }

  enum StarType: Int {
    case regular = 0
    case boxed = 1
  }

  enum Default {
    static let emptyBorderColor = UIColor.orange
    static let emptyColor = UIColor.clear
    static let fillMode = FillMode.full
    static let filledBorderColor = UIColor.orange
    static let borderWidth = 1 / UIScreen.main.scale
    static let filledColor = UIColor.orange
    static let minTouchRating: CGFloat = 1
    static let value: CGFloat = 2.2
    static let starMargin: CGFloat = 4
    static let starSize: CGFloat = 20
    static let textColor = UIColor.lightText
    static let textFont = UIFont.preferredFont(forTextStyle: UIFont.TextStyle.footnote)
    static let textMargin: CGFloat = 4
    static let textSize = textFont.pointSize
    static let starsCount = 5

    static let starType = StarType.boxed
    static let starPointsBoxSize: CGFloat = 100
    static let starPoints: [StarType: [CGPoint]] = {
      let regular: [CGPoint] = [
        CGPoint(x: 49.5, y: 0.0),
        CGPoint(x: 60.5, y: 35.0),
        CGPoint(x: 99.0, y: 35.0),
        CGPoint(x: 67.5, y: 58.0),
        CGPoint(x: 78.5, y: 92.0),
        CGPoint(x: 49.5, y: 71.0),
        CGPoint(x: 20.5, y: 92.0),
        CGPoint(x: 31.5, y: 58.0),
        CGPoint(x: 0.0, y: 35.0),
        CGPoint(x: 38.5, y: 35.0),
      ]

      let boxed: [CGPoint] = [
        CGPoint(x: 50.5, y: 22.2),
        CGPoint(x: 57.6, y: 45.6),
        CGPoint(x: 79.9, y: 45.6),
        CGPoint(x: 61.7, y: 58.1),
        CGPoint(x: 68.6, y: 78.8),
        CGPoint(x: 50.5, y: 66.0),
        CGPoint(x: 32.4, y: 78.8),
        CGPoint(x: 39.3, y: 58.1),
        CGPoint(x: 21.2, y: 45.6),
        CGPoint(x: 43.4, y: 45.6),
      ]

      return [.regular: regular, .boxed: boxed]
    }()
  }

  init() {}

  var borderWidth = Default.borderWidth
  var emptyBorderColor = Default.emptyBorderColor
  var emptyColor = Default.emptyColor
  var emptyImage: UIImage?
  var fillMode = Default.fillMode
  var filledBorderColor = Default.filledBorderColor
  var filledColor = Default.filledColor
  var filledImage: UIImage?
  var minTouchRating = Default.minTouchRating

  var textColors: [TextSide: UIColor] = [
    .left: Default.textColor,
    .right: Default.textColor,
    .top: Default.textColor,
    .bottom: Default.textColor,
  ]
  var textFonts: [TextSide: UIFont] = [
    .left: Default.textFont,
    .right: Default.textFont,
    .top: Default.textFont,
    .bottom: Default.textFont,
  ]
  var textSizes: [TextSide: CGFloat] = [
    .left: Default.textSize,
    .right: Default.textSize,
    .top: Default.textSize,
    .bottom: Default.textSize,
  ]
  var textMargins: [TextSide: CGFloat] = [
    .left: Default.textMargin,
    .right: Default.textMargin,
    .top: Default.textMargin,
    .bottom: Default.textMargin,
  ]

  var starMargin = Default.starMargin
  var starPoints = Default.starPoints[Default.starType]!
  var starSize = Default.starSize
  var starType = Default.starType
  var starsCount = Default.starsCount
}
