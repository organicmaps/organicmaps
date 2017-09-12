@objc(MWMSearchIndex)
final class SearchIndex: NSObject {
  fileprivate struct Item {
    let type: MWMSearchItemType
    let containerIndex: Int
  }

  fileprivate struct PositionItem {
    let item: Item
    var position: Int
  }

  private var positionItems: [PositionItem] = []
  private var items: [Item] = []

  var count: Int {
    return items.count
  }

  init(suggestionsCount: Int, resultsCount: Int) {
    for index in 0 ..< resultsCount {
      let type: MWMSearchItemType = index < suggestionsCount ? .suggestion : .regular
      let item = Item(type: type, containerIndex: index)
      positionItems.append(PositionItem(item: item, position: index))
    }
    super.init()
  }

  func addItem(type: MWMSearchItemType, prefferedPosition: Int, containerIndex: Int) {
    assert(type != .suggestion && type != .regular)
    let item = Item(type: type, containerIndex: containerIndex)
    positionItems.append(PositionItem(item: item, position: prefferedPosition))
  }

  func build() {
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

  func resultType(row: Int) -> MWMSearchItemType {
    return items[row].type
  }

  func resultContainerIndex(row: Int) -> Int {
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
