class BookingError(Exception):
    pass


class HTTPError(BookingError):
    pass


class AttemptsSpentError(BookingError):
    pass


class GettingMinPriceError(BookingError):
    pass
