import UIKit

func saveMultipleImages(_ images: [UIImage], placeId: Int64) -> [URL] {
  let fileManager = FileManager.default
  let documentsDirectory = fileManager.urls(for: .cachesDirectory, in: .userDomainMask)[0]
  
  return images.enumerated().compactMap { (index, image) in
    let fileName = "image_\(index)_placeId\(placeId).jpg"
    let fileURL = documentsDirectory.appendingPathComponent(fileName)
    
    guard let data = image.jpegData(compressionQuality: 0.01) else { return nil }
    
    do {
      try data.write(to: fileURL)
      return fileURL
    } catch {
      print("Error saving image \(fileName): \(error)")
      return nil
    }
  }
}

func retrieveMultipleImages(urls: [URL]) -> [UIImage] {
  return urls.compactMap { url in
    do {
      let imageData = try Data(contentsOf: url)
      return UIImage(data: imageData)
    } catch {
      print("Error retrieving image at \(url): \(error)")
      return nil
    }
  }
}
