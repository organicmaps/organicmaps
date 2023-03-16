protocol InfoMetadata {}

struct ListItemInfo {
  let type: String
  let metadata: InfoMetadata?
  
  init(type: String, metadata: InfoMetadata?) {
    self.type = type
    self.metadata = metadata
  }
}
