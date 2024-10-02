import SwiftUI
import Combine

struct FullscreenImageViewer: View {
  let imageUrls: [String]
  @State private var currentPage: Int
  @State private var scale: CGFloat = 1.0
  @State private var lastScale: CGFloat = 1.0
  @State private var offset: CGSize = .zero
  @State private var lastOffset: CGSize = .zero
  @GestureState private var magnifyBy = 1.0
  
  @Environment(\.presentationMode) var presentationMode
  
  init(selectedImageUrl: String, imageUrls: [String]) {
    self.imageUrls = imageUrls
    let initialIndex = imageUrls.firstIndex(of: selectedImageUrl) ?? 0
    _currentPage = State(initialValue: initialIndex)
  }
  
  var body: some View {
    GeometryReader { geometry in
      ZStack {
        // images
        SwiftUI.TabView(selection: $currentPage) {
          ForEach(imageUrls.indices, id: \.self) { index in
            ZoomableImageView(
              imageUrl: imageUrls[index],
              scale: $scale,
              offset: $offset
            )
            .tag(index)
          }
        }
        .tabViewStyle(PageTabViewStyle(indexDisplayMode: .never))
        .gesture(
          MagnificationGesture()
            .updating($magnifyBy) { currentState, gestureState, _ in
              gestureState = currentState
            }
            .onEnded { value in
              scale = min(max(1, scale * value), 3)
            }
        )
        .simultaneousGesture(
          DragGesture()
            .onChanged { value in
              if scale > 1 {
                offset = CGSize(
                  width: lastOffset.width + value.translation.width,
                  height: lastOffset.height + value.translation.height
                )
              }
            }
            .onEnded { _ in
              if scale > 1 {
                lastOffset = offset
              } else {
                offset = .zero
                lastOffset = .zero
              }
            }
        )
        
        // back button
        VStack {
          HStack {
            BackButtonWithText {
              presentationMode.wrappedValue.dismiss()
            }.padding(.leading, 16)
            Spacer()
          }
          Spacer()
        }
        
        //page indicator
        VStack {
          Spacer()
          HStack(spacing: 8) {
            ForEach(imageUrls.indices, id: \.self) { index in
              Circle()
                .fill(currentPage == index ? Color.primary : Color.primary.opacity(0.25))
                .frame(width: 8, height: 8)
            }
          }
        }
        .padding(.bottom, 16)
      }
    }
  }
}

struct ZoomableImageView: View {
  let imageUrl: String
  @Binding var scale: CGFloat
  @Binding var offset: CGSize
  @StateObject private var imageLoader = ImageLoader()
  
  var body: some View {
    Group {
      if let image = imageLoader.image {
        Image(uiImage: image)
          .resizable()
          .aspectRatio(contentMode: .fit)
          .scaleEffect(scale)
          .offset(offset)
      } else if imageLoader.isLoading {
        ProgressView()
      } else {
        Text(L("error"))
      }
    }
    .onAppear {
      imageLoader.load(fromURLString: imageUrl)
    }
  }
}

class ImageLoader: ObservableObject {
  @Published var image: UIImage?
  @Published var isLoading = false
  private var cancellable: AnyCancellable?
  
  func load(fromURLString urlString: String) {
    guard let url = URL(string: urlString) else { return }
    
    cancellable?.cancel()
    self.image = nil
    self.isLoading = true
    
    cancellable = URLSession.shared.dataTaskPublisher(for: url)
      .map { UIImage(data: $0.data) }
      .replaceError(with: nil)
      .receive(on: DispatchQueue.main)
      .sink { [weak self] in
        self?.image = $0
        self?.isLoading = false
      }
  }
}
