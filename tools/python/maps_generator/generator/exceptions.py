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
        args = p.args
        logs = p.output.name
        if p.error.name != logs:
            logs += " and " + p.error.name
        msg = f"The launch of {args.pop(0)} failed.\nArguments used: {' '.join(args)}\nSee details in {logs}"
        raise BadExitStatusError(msg)
