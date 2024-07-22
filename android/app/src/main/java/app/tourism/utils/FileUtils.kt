package app.tourism.utils

import android.content.Context
import android.graphics.Bitmap
import id.zelory.compressor.Compressor
import id.zelory.compressor.constraint.format
import id.zelory.compressor.constraint.size
import java.io.File

suspend fun saveToInternalStorage(files: List<File>, context: Context) {
    val filesDir = context.filesDir
    files.forEach { file ->
        val destinationFile = File(filesDir, file.name)
        file.inputStream().copyTo(destinationFile.outputStream())
    }
}

suspend fun compress(file: File, context: Context): File =
    Compressor.compress(context, file) {
        format(Bitmap.CompressFormat.JPEG)
        size(900_000)
    }