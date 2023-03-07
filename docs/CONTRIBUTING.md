# Contributing

Thank you for your interest in contributing to Organic Maps (OM)!

## How Can I Contribute?

There are many ways to contribute and OM needs a variety of talents: software engineers, graphic designers, translators, UI/UX experts, marketing/PR, etc.

### Donate

See https://organicmaps.app/donate/

### Bug Reports

The simplest way to contribute is to [submit an issue](https://github.com/organicmaps/organicmaps/issues).
Please check if there are no similar issues already submitted by someone else,
and give developers as much information as possible: OS and application versions,
list of actions leading to a bug, a log file produced by the app.

When using Organic Maps app on a device, use the built-in "Report a bug" option:
on Android it creates a new e-mail with a log file attached. Your issue will be processed much
faster if you send it to bugs@organicmaps.app. Enabling logs in Organic Maps settings on Android
before sending the bug report also helps us a lot with debugging.

### Feature Requests

If you have some ideas or want to request a new feature, please [start a discussion thread](https://github.com/organicmaps/organicmaps/discussions/categories/ideas).

### Translations

OM is available in 35 languages already, but some of them are incomplete and existing translations need regular updates as the app evolves.
See [translations instructions](TRANSLATIONS.md) for details.

### Map styling and icons

We strive to have a functional, cohesive and pleasant to the eye map rendering style.
There is always something to improve, add new map features, fine tune colors palette etc.
And every time we add a new map feature/POI we need a good and free-to-use icon.

See [styles and icons instructions](STYLES.md) for details.

### Code Contributions

Please follow instructions in [INSTALL.md](INSTALL.md) to set up your development environment
and check the [developer's guidelines](#developers-guidelines).
You will find a list of issues for new contributors [here](https://github.com/organicmaps/organicmaps/labels/Good%20first%20issue) to help you get started with simple tasks.

**Please do not ask for permission to work on the issue or to assign an issue to you**. We do not assign issues to first-time contributors. Any such comment notifies our contributors and the development team, and creates unnecessary noise that distracts us from the work.

## Submitting your changes

All contributions to Organic Maps repository should be submitted via
[Github pull requests](https://docs.github.com/en/pull-requests/collaborating-with-pull-requests/proposing-changes-to-your-work-with-pull-requests/creating-a-pull-request-from-a-fork)
and signed-off with the [Developers Certificate of Origin](#legal-requirements).

Each pull request is reviewed by OM maintainers to ensure its quality.
Sometimes the review process even for smallest commits can be
very thorough.

### Legal Requirements

When contributing to this project, you must agree that you have authored 100%
of the content, that you have the necessary rights to the content and that
the content you contribute may be provided under the project license.

To contribute you must assure that you have read and are following the rules
stated in the [Developers Certificate of Origin](DCO.md) (DCO). We have
borrowed this procedure from the Linux kernel project to improve tracking of
who did what, and for legal reasons.

To sign-off a patch, just add a line in the commit message saying:

    Signed-off-by: Some Developer <somedev@example.com>

Git has a flag that can sign a commit for you. An example using it is:

    git commit -s -m 'An example commit message'

Use your real name or on some rare cases a company email address, but we
disallow pseudonyms or anonymous contributions.

## Code of Conduct

The OM community abides by the [CNCF code of conduct](CODE_OF_CONDUCT.md).

## Developer's Guidelines

Please [learn how to use `git rebase`](https://git-scm.com/book/en/v2/Git-Branching-Rebasing) to amend your commits
and have a clean history for your changes/branches.
Do not close/recreate Pull Requests if you want to edit commits. Use `git rebase` and `git commit --amend`,
or any git tool with a graphical interface ([Fork for Mac](https://git-fork.com/) is quite good) to make clean,
logical commits, properly signed with [DCO](DCO.md).

- [Directories structure](STRUCTURE.md)
- [C++ Style Guide](CPP_STYLE.md).
- [Java Style Guide](JAVA_STYLE.md).
- [Objective-C Style Guide](OBJC_STYLE.md).
- [Pull Request Guide](PR_GUIDE.md).
- [How to write a commit message](COMMIT_MESSAGES.md).

## Questions?

For any questions about developing OM and relevant services -
virtually about anything related, please [contact us](COMMUNICATION.md),
we'll be happy to help.
