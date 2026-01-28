class MainTheme: Theme {
  override func register() {
    registerStyleSheet(GlobalStyleSheet.self)
    registerStyleSheet(PlacePageStyleSheet.self)
    registerStyleSheet(MapStyleSheet.self)
    registerStyleSheet(BookmarksStyleSheet.self)
    registerStyleSheet(SearchStyleSheet.self)
    registerStyleSheet(FontStyleSheet.self)
    registerStyleSheet(TextColorStyleSheet.self)
  }
}

