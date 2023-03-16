class DescriptionError(Exception):
    pass


class ParseError(DescriptionError):
    pass


class GettingError(DescriptionError):
    pass
