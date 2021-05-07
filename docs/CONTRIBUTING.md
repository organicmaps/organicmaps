# Contributing

Thank you for your interest in contributing to Organic Maps!

## How Can I Contribute?

There are many ways to contribute.

### Bug Reports

The simplest way to contribute is to [submit an issue](https://github.com/organicmaps/organicmaps/issues).Please give developers as much information as possible: OS and application versions,
list of actions leading to a bug, a log file produced by the app.

When using the Organic Maps app on a device, use the built-in "Report a bug" option:
it creates a new e-mail with a log file attached. Your issue will be processed much
faster if you send it to bugs@organicmaps.app.

### Feature Requests

If you have some ideas or want to request a new feature, please [start a discussion thread](https://github.com/organicmaps/organicmaps/discussions/categories/ideas).

### Translations

If you want to improve app translations or add more search synonyms, please update [strings.txt](https://github.com/organicmaps/organicmaps/blob/master/data/strings/strings.txt) file, run `./tools/unix/generate_localizations.sh` and create a [Pull Request](#pull-requests).

### Code Contributions

Please follow instructions in [INSTALL.md](INSTALL.md) to set up your development environment.
Create and submit a [Pull Request](#pull-requests) with your changes.

## Process

### Pull Requests

All contributions to Organic Maps source code should be submitted via github pull requests.
Each pull request is reviewed by OMaps maintainers, to ensure consistent code style
and quality. Sometimes the review process even for smallest commits can be
very thorough. Please follow [the developer guidelines](#guidelines).

### Legal Requirements

When contributing to this project, you must agree that you have authored 100%
of the content, that you have the necessary rights to the content and that
the content you contribute may be provided under the project license.

To contribute you must assure that you have read and are following the rules
stated in the [Developers Certificate of Origin](DCO.md) (DCO). We have
borrowed this procedure from the Linux kernel project to improve tracking of
who did what, and for legal reasons.

To sign-off a patch, just add a line in the commit message saying:

    Signed-off-by: Some Developer somedev@example.com

Git has a flag that can sign a commit for you. An example using it is:

    git commit -s -m 'An example commit message'

Use your real name or on some rare cases a company email address, but we
disallow pseudonyms or anonymous contributions.

### Code of Conduct

The Organic Maps community abides by the [CNCF code of conduct](CODE_OF_CONDUCT).

### Guidelines

- [C++ Style Guide](CPP_STYLE.md).
- [Objective-C Style Guide](OBJC_STYLE.md).
- [Pull Request Guide](PR_GUIDE.md).
- [How to write a commit message](COMMIT_MESSAGES.md).

## Questions?

For any questions about developing Organic Maps and relevant services -
virtually about anything related, please [contact us](COMMUNICATION.md),
we'll be happy to help.
