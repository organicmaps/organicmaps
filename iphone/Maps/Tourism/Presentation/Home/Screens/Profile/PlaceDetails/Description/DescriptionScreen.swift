import SwiftUI

struct DescriptionScreen: View {
  var description: String?
  var onCreateRoute: (() -> Void)?
  
  var body: some View {
    ZStack {
      // description
      if let description = description {
        ScrollView {
          VStack(alignment: .leading) {
            VerticalSpace(height: 16)
            description.htmlToAttributedString()
              .textStyle(.b1)
          }
          VerticalSpace(height: 100) // it's needed for visibility over the button below
        }
      } else {
        EmptyUI()
      }
      
      // create route button
      if let onCreateRoute = onCreateRoute {
        VStack() {
          Spacer()
          
          PrimaryButton(
            label: NSLocalizedString("show_route", comment: ""),
            onClick: onCreateRoute
          )
          .padding(.bottom, 32)
          .frame(maxWidth: .infinity, alignment: .bottom)
        }.frame(minHeight: 0, maxHeight: .infinity)
      }
    }.padding(.horizontal, 16)
  }
}

extension String {
  func htmlToAttributedString() -> Text {
    // Assuming you have a function to convert HTML to an attributed string
    // Here's a basic version:
    guard let data = self.data(using: .utf8) else { return Text(self) }
    
    if let attributedString = try? NSAttributedString(
      data: data,
      options: [.documentType: NSAttributedString.DocumentType.html, .characterEncoding: String.Encoding.utf8.rawValue],
      documentAttributes: nil
    ) {
      return Text(attributedString.string)
    }
    
    return Text(self)
  }
}
