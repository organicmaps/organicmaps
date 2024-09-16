import SwiftUI

struct GalleryScreen: View {
  let urls: [String]?
  
  let secondRowHeight = 100.0
  let shape = RoundedRectangle(cornerRadius: 8)
  @State var goToAllGalleryScreen = false
  
  var body: some View {
    if let urls = urls, !urls.isEmpty {
      VStack {
        LoadImageView(url: urls.first)
          .frame(maxWidth: UIScreen.main.bounds.width - 32, minHeight: 200, maxHeight: 200)
          .clipShape(shape)
        
        VerticalSpace(height: 16)
        
        HStack(spacing: 16) {
          if urls.count > 1 {
            LoadImageView(url: urls[1])
              .frame(height: secondRowHeight)
              .clipShape(shape)
              .aspectRatio(1, contentMode: .fit)
            
            if urls.count > 2 {
              NavigationLink(destination: AllPicsScreen(urls: urls)) {
                ZStack {
                  LoadImageView(url: urls[2])
                    .frame(height: secondRowHeight)
                  
                  if urls.count > 3 {
                    SwiftUI.Color.black.opacity(0.5)
                      .frame(height: secondRowHeight)
                      .clipShape(shape)
                    
                    Text("+\(urls.count - 3)")
                      .font(.headline)
                      .foregroundColor(.white)
                  }
                }
                .clipShape(shape)
                .aspectRatio(1, contentMode: .fit)
              }
            }
          }
        }
        Spacer()
      }.padding(.horizontal, 16)
    } else {
      EmptyUI()
    }
  }
}
