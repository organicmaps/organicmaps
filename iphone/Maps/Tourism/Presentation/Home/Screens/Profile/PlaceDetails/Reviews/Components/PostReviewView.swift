import SwiftUI
import Combine
import PhotosUI

struct PostReviewView: View {
  @ObservedObject var postReviewVM: PostReviewViewModel
  let placeId: Int64
  let onPostReviewSuccess: () -> Void
  
  @State private var showImagePicker = false
  
  var body: some View {
    ScrollView {
      VStack {
        Spacer().frame(height: 32)
        
        Text(L("review_title"))
          .font(.title)
        Spacer().frame(height: 32)
        
        VStack(alignment: .center) {
          Text(L("tap_to_rate"))
            .font(.body)
          Spacer().frame(height: 4)
          RatingBarView(rating: $postReviewVM.rating, size: 25)
        }
        Spacer().frame(height: 16)
        
        MultilineTextField(L("text"), text: $postReviewVM.comment, minHeight: 80)
        Spacer().frame(height: 16)
        
        // Display the selected images
        FlowStack(data: postReviewVM.files, spacing: 16, alignment: .center) { file in
          ImagePreviewView(image: file) {
            postReviewVM.removeFile(file)
          }
        }
        Spacer().frame(height: 32)
        
        if(postReviewVM.files.count < 10) {
          VStack(alignment: .leading) {
            PrimaryButton(
              label: L("upload_photo"),
              onClick: {
                showImagePicker = true
              },
              isLoading: postReviewVM.isPosting
            )
            Spacer().frame(height: 4)
            Text(L("images_number_warning"))
              .textStyle(TextStyle.b1)
              .foregroundColor(Color.hint)
            Spacer().frame(height: 16)
          }
        }
        
        PrimaryButton(
          label: L("send"),
          onClick: {
            postReviewVM.postReview(placeId: placeId)
          },
          isLoading: postReviewVM.isPosting
        )
        
        Spacer().frame(height: 64)
      }
      .padding(.horizontal, 16)
      .onReceive(postReviewVM.uiEvents) { event in
        switch event {
        case .closeReviewBottomSheet:
          onPostReviewSuccess()
        case .showToast(let message):
          // TODO: cmon
          print(message)
        }
      }
      .sheet(isPresented: $showImagePicker) {
        MultiImagePicker(selectedImages: $postReviewVM.files)
      }
    }
  }
}

struct ImagePreviewView: View {
  let image: UIImage
  let onDelete: () -> Void
  
  var body: some View {
    ZStack(alignment: .topTrailing) {
      Image(uiImage: image)
        .resizable()
        .scaledToFill()
        .frame(width: 100, height: 100)
        .cornerRadius(12)
      Button(action: onDelete) {
        Image(systemName: "xmark.circle.fill")
          .foregroundColor(.red)
      }
      .offset(x: 10, y: -10)
    }
  }
}


struct MultilineTextField: View {
    @Binding var text: String
    let placeholder: String
    let minHeight: CGFloat
    
    init(_ placeholder: String, text: Binding<String>, minHeight: CGFloat = 100) {
        self._text = text
        self.placeholder = placeholder
        self.minHeight = minHeight
    }
    
    var body: some View {
        ZStack(alignment: .topLeading) {
            TextEditor(text: $text)
                .frame(minHeight: minHeight)
                .padding(4)
            
            if text.isEmpty {
                Text(placeholder)
                .foregroundColor(SwiftUI.Color(.placeholderText))
                    .padding(.horizontal, 8)
                    .padding(.vertical, 12)
            }
        }
        .overlay(
            RoundedRectangle(cornerRadius: 8)
              .stroke(SwiftUI.Color.gray.opacity(0.2), lineWidth: 1)
        )
    }
}
