package app.tourism.domain.models.resource

sealed class Resource<T>(val data: T? = null, val message: String? = null) {
    class Idle<T>: Resource<T>()
    class Loading<T>(data: T? = null): Resource<T>(data)
    class Success<T>(data: T?, message: String? = null): Resource<T>(data)
    class Error<T>(message: String, data: T? = null): Resource<T>(data, message)
}
