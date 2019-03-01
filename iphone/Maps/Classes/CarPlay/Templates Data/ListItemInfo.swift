@available(iOS 12.0, *)
protocol InfoMetadata {}

@available(iOS 12.0, *)
struct ListItemInfo {
  let type: String
  let metadata: InfoMetadata?
  
  init(type: String, metadata: InfoMetadata?) {
    self.type = type
    self.metadata = metadata
  }
}
