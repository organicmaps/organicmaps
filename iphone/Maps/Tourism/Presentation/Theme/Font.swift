import SwiftUI
// MARK: - used in SwiftUI
extension Font {
    static func black(size: CGFloat) -> Font {
        return Font.custom("Gilroy-Black", size: size)
    }
    
    static func bold(size: CGFloat) -> Font {
        return Font.custom("Gilroy-Bold", size: size)
    }
    
    static func extraBold(size: CGFloat) -> Font {
        return Font.custom("Gilroy-ExtraBold", size: size)
    }
    
    static func heavy(size: CGFloat) -> Font {
        return Font.custom("Gilroy-Heavy", size: size)
    }
    
    static func light(size: CGFloat) -> Font {
        return Font.custom("Gilroy-Light", size: size)
    }
    
    static func medium(size: CGFloat) -> Font {
        return Font.custom("Gilroy-Medium", size: size)
    }
    
    static func regular(size: CGFloat) -> Font {
        return Font.custom("Gilroy-Regular", size: size)
    }
    
    static func semiBold(size: CGFloat) -> Font {
        return Font.custom("Gilroy-SemiBold", size: size)
    }
    
    static func thin(size: CGFloat) -> Font {
        return Font.custom("Gilroy-Thin", size: size)
    }
    
    static func ultraLight(size: CGFloat) -> Font {
        return Font.custom("Gilroy-UltraLight", size: size)
    }
}

extension TextStyle {
    static let genericStyle = TextStyle(
        font: .regular(size: 16.0),
        lineHeight: 18
    )
    
    static let humongous = TextStyle(
        font: .extraBold(size: 36.0),
        lineHeight: 40
    )
    
    static let h1 = TextStyle(
        font: .semiBold(size: 32.0),
        lineHeight: 36
    )
    
    static let h2 = TextStyle(
        font: .semiBold(size: 24.0),
        lineHeight: 36
    )
    
    static let h3 = TextStyle(
        font: .semiBold(size: 20.0),
        lineHeight: 22
    )
    
    static let h4 = TextStyle(
        font: .medium(size: 16.0),
        lineHeight: 18
    )
    
    static let b1 = TextStyle(
        font: .regular(size: 14.0),
        lineHeight: 16
    )
    
    static let b2 = TextStyle(
        font: .regular(size: 12.0),
        lineHeight: 14
    )
    
    static let b3 = TextStyle(
        font: .regular(size: 10.0),
        lineHeight: 12
    )
}


struct TextStyle {
    let font: Font
    let lineHeight: CGFloat
}

extension Text {
    func textStyle(_ style: TextStyle) -> some View {
        let calibrationFactor: CGFloat = 0.2
        return self
            .font(style.font)
            .lineSpacing(style.lineHeight * calibrationFactor)
    }
}


// MARK: - used in UIKit

class UIKitFont {
  // MARK: - Font by Weights
  class func black(size: CGFloat) -> UIFont {
    return getCustomFont(withName: "Gilroy-Black", size: size)
  }
  
  class func bold(size: CGFloat) -> UIFont {
    return getCustomFont(withName: "Gilroy-Bold", size: size)
  }
  
  class func extraBold(size: CGFloat) -> UIFont {
    return getCustomFont(withName: "Gilroy-ExtraBold", size: size)
  }
  
  class func heavy(size: CGFloat) -> UIFont {
    return getCustomFont(withName: "Gilroy-Heavy", size: size)
  }
  
  class func light(size: CGFloat) -> UIFont {
    return getCustomFont(withName: "Gilroy-Light", size: size)
  }
  
  class func medium(size: CGFloat) -> UIFont {
    return getCustomFont(withName: "Gilroy-Medium", size: size)
  }
  
  class func regular(size: CGFloat) -> UIFont {
    return getCustomFont(withName: "Gilroy-Regular", size: size)
  }
  
  class func semiBold(size: CGFloat) -> UIFont {
    return getCustomFont(withName: "Gilroy-SemiBold", size: size)
  }
  
  class func thin(size: CGFloat) -> UIFont {
    return getCustomFont(withName: "Gilroy-Thin", size: size)
  }
  
  class func ultraLight(size: CGFloat) -> UIFont {
    return getCustomFont(withName: "Gilroy-UltraLight", size: size)
  }
  
  // MARK: - Font by Styles
  static let genericStyle = UIKitTextStyle(
    font: regular(size: 16.0),
    lineHeight: 18
  )
  
  static let humongous = UIKitTextStyle(
    font: extraBold(size: 36.0),
    lineHeight: 40
  )
  
  static let h1 = UIKitTextStyle(
    font: semiBold(size: 32.0),
    lineHeight: 36
  )
  
  static let h2 = UIKitTextStyle(
    font: semiBold(size: 24.0),
    lineHeight: 36
  )
  
  static let h3 = UIKitTextStyle(
    font: semiBold(size: 20.0),
    lineHeight: 22
  )
  
  static let h4 = UIKitTextStyle(
    font: medium(size: 16.0),
    lineHeight: 18
  )
  
  static let b1 = UIKitTextStyle(
    font: regular(size: 14.0),
    lineHeight: 16
  )
  
  static let b2 = UIKitTextStyle(
    font: regular(size: 12.0),
    lineHeight: 14
  )
  
  static let b3 = UIKitTextStyle(
    font: regular(size: 10.0),
    lineHeight: 12
  )
  
  // MARK: - funcs
  private class func getCustomFont(withName name: String, size: CGFloat) -> UIFont {
    if let font = UIFont(name: name, size: size) {
      return font
    }
    return UIFont.systemFont(ofSize: size)
  }
  
  static func applyStyle(to label: UILabel, style: UIKitTextStyle) {
    label.font = style.font
    label.adjustsFontForContentSizeCategory = true
    let lineHeight = style.lineHeight
    let paragraphStyle = NSMutableParagraphStyle()
    paragraphStyle.lineSpacing = lineHeight - label.font.lineHeight
    
    label.adjustsFontForContentSizeCategory = true
    label.font = UIFontMetrics(forTextStyle: .body).scaledFont(for: style.font)
    label.attributedText = NSAttributedString(string: label.text ?? "", attributes: [.paragraphStyle: paragraphStyle])
  }
}

struct UIKitTextStyle {
  let font: UIFont
  let lineHeight: CGFloat
}
