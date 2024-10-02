import SwiftUI

struct GalleryScreen: View {
  let urls: [String]?
  
  let secondRowHeight = 130.0
  let shape = RoundedRectangle(cornerRadius: 8)
  @State var goToAllGalleryScreen = false
  
  var body: some View {
    if let urls = urls, !urls.isEmpty {
      VStack {
        NavigationLink(destination: FullscreenImageViewer(selectedImageUrl: urls[0], imageUrls: urls)) {
          LoadImageView(url: urls.first)
            .frame(maxWidth: UIScreen.main.bounds.width - 32, minHeight: 200, maxHeight: 200)
            .clipShape(shape)
        }
        
        VerticalSpace(height: 16)
        
        GeometryReader { geometry in
          let secondColumnWidth = geometry.size.width / 2 - 8
          
          HStack(spacing: 16) {
            if urls.count > 1 {
              NavigationLink(destination: FullscreenImageViewer(selectedImageUrl: urls[1], imageUrls: urls)) {
                LoadImageView(url: urls[1])
                  .frame(width: secondColumnWidth, height: secondRowHeight)
                  .clipShape(shape)
              }
              if (urls.count == 2) { Spacer() }
              
              if urls.count > 2 {
                NavigationLink(destination: AllPicsScreen(urls: urls)) {
                  ZStack {
                    LoadImageView(url: urls[2])
                    
                    if urls.count > 3 {
                      SwiftUI.Color.black.opacity(0.5)
                        .frame(height: secondRowHeight)
                        .clipShape(shape)
                      
                      Text("+\(urls.count - 3)")
                        .font(.headline)
                        .foregroundColor(.white)
                    }
                  }
                  .frame(width: secondColumnWidth, height: secondRowHeight)
                  .clipShape(shape)
                }
              }
            }
          }.frame(width: geometry.size.width)
        }
        Spacer()
      }.padding(.horizontal, 16)
    } else {
      EmptyUI()
    }
  }
}
