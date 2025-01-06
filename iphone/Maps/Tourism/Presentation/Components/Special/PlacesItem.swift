import SwiftUI
import WebKit

struct PlacesItem: View {
  let place: PlaceShort
  let onPlaceClick: (PlaceShort) -> Void
  let onFavoriteChanged: (Bool) -> Void
  
  private let height: CGFloat = 130
  
  var body: some View {
    HStack {
      LoadImageView(url: place.cover)
        .frame(width: height, height: height)
        .clipShape(RoundedRectangle(cornerRadius: 20))
      
      VStack(alignment: .leading, spacing: 8) {
        HStack {
          Text(place.name)
            .font(.semiBold(size: 20))
            .foregroundColor(Color.onBackground)
            .lineLimit(1)
          
          Spacer()
          
          Button(action: {
            onFavoriteChanged(!place.isFavorite)
          }) {
            Image(systemName: place.isFavorite ? "heart.fill" : "heart")
              .foregroundColor(Color.heartRed)
          }
        }
        
        HStack {
          Text(String(format: "%.1f", place.rating ?? 0.0))
            .font(.regular(size: 14))
            .foregroundColor(Color.onBackground)
          Image(systemName: "star.fill")
            .foregroundColor(Color.starYellow)
            .font(.system(size: 12))
        }
        
        if let excerpt = place.excerpt {
          AttributedText(excerpt)
            .font(.regular(size: 14))
            .foregroundColor(Color.onBackground)
            .lineLimit(3)
        }
      }
      .padding(8)
    }
    .frame(height: height)
    .background(Color.background)
    .cornerRadius(20)
    .overlay(
      RoundedRectangle(cornerRadius: 20)
        .stroke(Color.border, lineWidth: 1)
    )
    .onTapGesture {
      onPlaceClick(place)
    }
  }
}

struct HTMLStringView: UIViewRepresentable {
    let htmlContent: String

    func makeUIView(context: Context) -> WKWebView {
        return WKWebView()
    }

    func updateUIView(_ uiView: WKWebView, context: Context) {
        uiView.loadHTMLString(htmlContent, baseURL: nil)
    }
}



/**
 AttributedText is a view for displaying some HTML-tagged text using SwiftUI Text View.
 
 - warning: **Only single-word tags are supported**. Tags with more than one word or
 containing any characters besides **letters** or **numbers** are ignored and not removed.
 
 # Notes
 1. Basic modifiers can still be applied, such as changing the font and color of the text.
 2. Handles unopened/unclosed tags.
 3. Supports overlapping tags.
 4. Deletes tags that have no modifiers.
 5. Does **not** handle HTML characters such as `&amp;`.
 
 # Example
 ```
 AttributedText("This is <b>bold</b> and <i>italic</i> text.")
     .foregroundColor(.blue)
     .font(.title)
     .padding()
 ```
 */
public struct AttributedText: View {
    /// Set of supported tags and associated modifiers. This is used by default for all AttributedText
    /// instances except those for which this parameter is defined in the initializer.
    public static var tags: Dictionary<String, (Text) -> (Text)> = [
        // This modifier set is presented just for reference.
        // Set the necessary attributes and modifiers for your needs before use.
        "h1": { $0.font(.largeTitle) },
        "h2": { $0.font(.title) },
        "h3": { $0.font(.headline) },
        "h4": { $0.font(.subheadline) },
        "h5": { $0.font(.callout) },
        "h6": { $0.font(.caption) },
        
        "i": { $0.italic() },
        "u": { $0.underline() },
        "s": { $0.strikethrough() },
        "b": { $0.fontWeight(.bold) },
        
        "sup": { $0.baselineOffset(10).font(.footnote) },
        "sub": { $0.baselineOffset(-10).font(.footnote) }
    ]
    /// Parser formatted text.
    private let text: Text

    /**
     Creates a text view that displays formatted content.
     
     - parameter htmlString: HTML-tagged string.
     - parameter tags: Set of supported tags and associated modifiers for a particular instance.
     */
    public init(_ htmlString: String, tags: Dictionary<String, (Text) -> (Text)>? = nil) {
        let parser = HTML2TextParser(htmlString, availableTags: tags == nil ? AttributedText.tags : tags!)
        parser.parse()
        text = parser.formattedText
    }

    public var body: some View {
        text
    }
}

struct AttributedText_Previews: PreviewProvider {
    static var previews: some View {
        AttributedText("This is <b>bold</b> and <i>italic</i> text.")
            .foregroundColor(.blue)
            .font(.title)
            .padding()
    }
}


/**
 Parser for converting HTML-tagged text to SwiftUI Text View.
 
 - warning: **Only single-word tags are supported**. Tags with more than one word or
 containing any characters besides **letters** or **numbers** are ignored and not removed.
 
 # Notes:
 1. Handles unopened/unclosed tags.
 2. Deletes tags that have no modifiers.
 3. Does **not** handle HTML characters, for example `&lt;`.
 */
internal class HTML2TextParser {
    /// The result of the parser's work.
    internal private(set) var formattedText = Text("")
    /// HTML-tagged text.
    private let htmlString: String
    /// Set of currently active tags.
    private var tags: Set<String> = []
    /// Set of supported tags and associated modifiers.
    private let availableTags: Dictionary<String, (Text) -> (Text)>

    /**
     Creates a new parser instance.
     
     - parameter htmlString: HTML-tagged string.
     - parameter availableTags: Set of supported tags and associated modifiers.
     */
    internal init(_ htmlString: String, availableTags: Dictionary<String, (Text) -> (Text)>) {
        self.htmlString = htmlString
        self.availableTags = availableTags
    }

    /// Starts the text parsing process. The results of this method will be placed in the `formattedText` variable.
    internal func parse() {
        var tag: String? = nil
        var endTag: Bool = false
        var startIndex = htmlString.startIndex
        var endIndex = htmlString.startIndex

        for index in htmlString.indices {
            switch htmlString[index] {
            case "<":
                tag = String()
                endIndex = index
                continue

            case "/":
                if index != htmlString.startIndex && htmlString[htmlString.index(before: index)] == "<" {
                    endTag = true
                } else {
                    tag = nil
                }
                continue

            case ">":
                if let tag = tag {
                    addChunkOfText(String(htmlString[startIndex..<endIndex]))
                    if endTag {
                        tags.remove(tag.lowercased())
                        endTag = false
                    } else {
                        tags.insert(tag.lowercased())
                    }
                    startIndex = htmlString.index(after: index)
                }
                tag = nil
                continue

            default:
                break
            }

            if tag != nil {
                if htmlString[index].isLetter || htmlString[index].isHexDigit {
                    tag?.append(htmlString[index])
                } else {
                    tag = nil
                }
            }
        }

        endIndex = htmlString.endIndex
        if startIndex != endIndex {
            addChunkOfText(String(htmlString[startIndex..<endIndex]))
        }
    }

    private func addChunkOfText(_ string: String) {
        guard !string.isEmpty else { return }
        var textChunk = Text(string)

        for tag in tags {
            if let action = availableTags[tag] {
                textChunk = action(textChunk)
            }
        }

        formattedText = formattedText + textChunk
    }
}
