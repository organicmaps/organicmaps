package app.tourism.utils

fun <T> makeLongListOfTheSameItem(item: T, itemsNum: Int = 20): List<T> {
    val list = mutableListOf<T>()
    repeat(itemsNum) { list.add(item) }
    return list
}