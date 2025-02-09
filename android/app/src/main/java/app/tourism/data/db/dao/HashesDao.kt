package app.tourism.data.db.dao

import androidx.room.Dao
import androidx.room.Insert
import androidx.room.OnConflictStrategy
import androidx.room.Query
import app.tourism.data.db.entities.HashEntity

@Dao
interface HashesDao {
    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun insertHash(hash: HashEntity)

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun insertHashes(hashes: List<HashEntity>)

    @Query("SELECT * FROM hashes WHERE categoryId = :id")
    suspend fun getHash(id: Long): HashEntity?

    @Query("SELECT * FROM hashes")
    suspend fun getHashes(): List<HashEntity>

    @Query("DELETE FROM hashes")
    suspend fun deleteHashes()
}
