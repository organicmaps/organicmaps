@objc
final class SearchIndex: NSObject {
  fileprivate struct Item {
    let type: SearchItemType
    let containerIndex: Int
  }

  fileprivate struct PositionItem {
    let item: Item
    var position: Int
  }

  private var positionItems: [PositionItem] = []
  private var items: [Item] = []

  @objc var count: Int {
    return items.count
  }

  @objc init(suggestionsCount: Int, resultsCount: Int) {
    for index in 0 ..< resultsCount {
      let type: SearchItemType = index < suggestionsCount ? .suggestion : .regular
      let item = Item(type: type, containerIndex: index)
      positionItems.append(PositionItem(item: item, position: index))
    }
    super.init()
  }

  func addItem(type: SearchItemType, prefferedPosition: Int, containerIndex: Int) {
    assert(type != .suggestion && type != .regular)
    let item = Item(type: type, containerIndex: containerIndex)
    positionItems.append(PositionItem(item: item, position: prefferedPosition))
  }

  @objc func build() {
    positionItems.sort(by: >)
    var itemsDict: [Int: Item] = [:]
    positionItems.forEach { item in
      var position = item.position
      while itemsDict[position] != nil {
        position += 1
      }
      itemsDict[position] = item.item
    }

    items.removeAll()
    let keys = itemsDict.keys.sorted()
    for index in 0 ..< keys.count {
      let key = keys[index]
      if index == key {
        items.append(itemsDict[key]!)
      }
    }
  }

  @objc func resultType(row: Int) -> SearchItemType {
    return items[row].type
  }

  @objc func resultContainerIndex(row: Int) -> Int {
    return items[row].containerIndex
  }
}

extension SearchIndex.PositionItem: Equatable {
  static func ==(lhs: SearchIndex.PositionItem, rhs: SearchIndex.PositionItem) -> Bool {
    let lhsCache = lhs.item
    let rhsCache = rhs.item
    return lhsCache.type == rhsCache.type &&
      lhs.position == rhs.position &&
      lhsCache.containerIndex == rhsCache.containerIndex
  }
}

extension SearchIndex.PositionItem: Comparable {
  static func <(lhs: SearchIndex.PositionItem, rhs: SearchIndex.PositionItem) -> Bool {
    let lhsCache = lhs.item
    let rhsCache = rhs.item
    guard lhsCache.type == rhsCache.type else {
      return lhsCache.type.rawValue < rhsCache.type.rawValue
    }
    guard lhs.position == rhs.position else {
      return lhs.position > rhs.position
    }
    return lhsCache.containerIndex < rhsCache.containerIndex
  }
}
