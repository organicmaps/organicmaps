import Foundation

@objc class FileManagerHelper: NSObject {
    
  
  @objc static func copyProjectFileToDocuments(fileName: String, fileExtension: String, toSubdirectory subdirectory: String) {
      let fileManager = FileManager.default

      guard let bundleURL = Bundle.main.url(forResource: fileName, withExtension: fileExtension) else {
          print("File not found in bundle.")
          return
      }

      if let documentsDirectory = fileManager.urls(for: .documentDirectory, in: .userDomainMask).first {
          let subdirectoryURL = documentsDirectory.appendingPathComponent(subdirectory)
          
          if !fileManager.fileExists(atPath: subdirectoryURL.path) {
              do {
                  try fileManager.createDirectory(at: subdirectoryURL, withIntermediateDirectories: true, attributes: nil)
                  print("Created directory: \(subdirectoryURL.path)")
              } catch {
                  print("Error creating directory: \(error.localizedDescription)")
                  return
              }
          }

          let destinationURL = subdirectoryURL.appendingPathComponent("\(fileName).\(fileExtension)")

          do {
              if fileManager.fileExists(atPath: destinationURL.path) {
                  print("File already exists at: \(destinationURL.path)")
              } else {
                  try fileManager.copyItem(at: bundleURL, to: destinationURL)
                  print("File copied successfully to: \(destinationURL.path)")
              }
          } catch {
              print("Error copying file: \(error.localizedDescription)")
          }
      }
  }
}

