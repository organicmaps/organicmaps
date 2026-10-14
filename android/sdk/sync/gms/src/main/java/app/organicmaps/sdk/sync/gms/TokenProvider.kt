package app.organicmaps.sdk.sync.gms

/** OAuth access tokens for Drive requests. */
interface TokenProvider {
    /** Throws CloudException(AuthRequired) when the user has to sign in again. */
    suspend fun token(): String

    /** Drops a token that the server rejected, so the next [token] call fetches a fresh one. */
    suspend fun invalidate(token: String)
}
