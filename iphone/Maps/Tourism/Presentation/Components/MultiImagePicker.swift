import SwiftUI
import PhotosUI

struct MultiImagePicker: UIViewControllerRepresentable {
  @Environment(\.presentationMode) private var presentationMode
  @Binding var selectedImages: [UIImage]
  let limit = 10
  
  func makeUIViewController(context: Context) -> PHPickerViewController {
    var config = PHPickerConfiguration()
    config.selectionLimit = limit
    config.filter = .images
    
    let picker = PHPickerViewController(configuration: config)
    picker.delegate = context.coordinator
    return picker
  }
  
  func updateUIViewController(_ uiViewController: PHPickerViewController, context: Context) {
    if(self.selectedImages.count > limit) {
      let numOfRedundantImages = self.selectedImages.count - limit
      selectedImages.removeLast(numOfRedundantImages)
    }
  }
  
  func makeCoordinator() -> Coordinator {
    Coordinator(self)
  }
  
  class Coordinator: NSObject, PHPickerViewControllerDelegate {
    var parent: MultiImagePicker
    
    init(_ parent: MultiImagePicker) {
      self.parent = parent
    }
    
    func picker(_ picker: PHPickerViewController, didFinishPicking results: [PHPickerResult]) {
      parent.presentationMode.wrappedValue.dismiss()
      
      for result in results {
        if result.itemProvider.canLoadObject(ofClass: UIImage.self) {
          result.itemProvider.loadObject(ofClass: UIImage.self) { (image, error) in
            if let image = image as? UIImage {
              DispatchQueue.main.async {
                self.parent.selectedImages.append(image)
              }
            }
          }
        }
      }
    }
  }
}
