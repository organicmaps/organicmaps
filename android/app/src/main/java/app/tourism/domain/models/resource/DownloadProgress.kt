package app.tourism.domain.models.resource

sealed class DownloadProgress(val stats: DownloadStats? = null, val message: String? = null) {
    class Idle : DownloadProgress()
    class Loading(stats: DownloadStats) : DownloadProgress(stats)
    class Finished(stats: DownloadStats, message: String? = null) : DownloadProgress(stats, message)
    class Error(message: String) : DownloadProgress(message = message)
}

class DownloadStats(
    val filesTotalNum: Int,
    var filesDownloaded: Int,
    var filesFailedToDownload: Int,
) {
    var percentagesCompleted: Int = 0

    init {
        updatePercentage()
    }

    fun updatePercentage() {
        percentagesCompleted = calculatePercentage()
    }

    fun isAllFilesProcessed() =
        filesTotalNum == filesDownloaded + filesFailedToDownload


    private fun calculatePercentage(): Int {
        return if (filesTotalNum == 0) 0 else (filesDownloaded * 100) / filesTotalNum
    }

    override fun toString(): String {
        return "DownloadStats(percentagesCompleted=$percentagesCompleted, filesDownloaded=$filesDownloaded, filesTotalNum=$filesTotalNum, filesFailedToDownload=$filesFailedToDownload)"
    }
}