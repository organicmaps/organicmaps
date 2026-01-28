struct SearchResultInfo: InfoMetadata {
  let originalRow: Int
  
  init(originalRow: Int) {
    self.originalRow = originalRow
  }
}
