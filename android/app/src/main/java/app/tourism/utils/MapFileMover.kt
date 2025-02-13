package app.tourism.utils

import android.content.Context
import android.util.Log
import app.organicmaps.BuildConfig
import java.io.BufferedInputStream
import java.io.BufferedOutputStream
import java.io.File
import java.io.FileOutputStream
import java.io.IOException

private const val TAG = "MapFileMover"

private const val SOURCE_FILE_NAME = "tajikistan.mwm"
private const val SOURCE_FILE_NEW_NAME = "Tajikistan.mwm"

class MapFileMover {

    private fun moveFile(context: Context, destinationPath: String): Boolean {
        return try {
            val destinationFile = File(destinationPath, SOURCE_FILE_NEW_NAME)

            val inputStream = BufferedInputStream(context.assets.open(SOURCE_FILE_NAME))
            val outputStream = BufferedOutputStream(FileOutputStream(destinationFile))

            val buffer = ByteArray(1024)
            var read: Int
            while (inputStream.read(buffer).also { read = it } != -1) {
                outputStream.write(buffer, 0, read)
            }

            inputStream.close()
            outputStream.flush()
            outputStream.close()

            Log.d(TAG, "File moved successfully to: ${destinationFile.absolutePath}")

            true
        } catch (e: IOException) {
            e.printStackTrace()
            Log.e(TAG, "Error moving file: ${e.message}")
            false
        }
    }

    fun main(context: Context): Boolean {
        val higherPath = "/storage/emulated/0/Android/data/tj.tourism.rebus"
        val lowerPath = "/files/240429/"
        var destinationPath = higherPath + lowerPath
        if (BuildConfig.BUILD_TYPE == "debug") {
            destinationPath = higherPath + ".debug" + lowerPath
        }

        val destinationDir = File(destinationPath)
        if (!destinationDir.exists()) {
            if (!destinationDir.mkdirs()) {
                Log.e(TAG, "Failed to create destination directory.")
                return false
            }
        }

        return moveFile(context, destinationPath)
    }
}
