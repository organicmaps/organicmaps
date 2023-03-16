import os


class MapsGeneratorError(Exception):
    pass


class OptionNotFound(MapsGeneratorError):
    pass


class ValidationError(MapsGeneratorError):
    pass


class ContinueError(MapsGeneratorError):
    pass


class SkipError(MapsGeneratorError):
    pass


class BadExitStatusError(MapsGeneratorError):
    pass


class ParseError(MapsGeneratorError):
    pass


class FailedTest(MapsGeneratorError):
    pass


def wait_and_raise_if_fail(p):
    if p.wait() != os.EX_OK:
        raise BadExitStatusError(f"The launch of {' '.join(p.args)} failed.")
