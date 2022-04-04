# Pull Request Guide

## Writing good PR:

- PR should be small and reflect only one idea, a feature, a bugfix, or a refactoring. In most cases, a PR of 100 lines or less is considered as a PR of normal size and a PR of 1000 lines as big one
- If a PR implements two different features, refactoring, or bugfixes, it should be split into several PRs
- The description field of every PR should contain a description, links to a ticket or/and links to the confluence
- If a PR implements a complex math task, the description should contain a link to the proof of the solution or a link to a trusted source
- All PRs should contain links to the corresponding tickets. The exception is refactoring if there's nothing to test after it. The reason for that is that every feature or bugfix should be tested by testers after it's implemented
- Every commit of all PRs should be compilable under all platforms. All tests should pass. So if changing of code breaks unit or integration tests these tests should be fixed in the same commit
- Every commit should reflect a completed idea and have an understandable comment. Review fixes should be merged into one commit
- New functionality and unit or integration tests for it should be developed in the same PR
- When some source files are changed and then some other source files based on them are auto-generated, they should be committed in different commits. For example, one commit changes strings.txt and another one contains generated files
- Most commits and PRs should have prefixes in square brackets depending on the changed subsystem. For example, [routing], [generator], or [android]. Commits and PRs may have several prefixes
- A PR which should not be merged after review should be marked as a draft.
- All commits and PR captions should be written in English. PR description and PR comments may be written in other languages
- All PRs should be approved by at least two reviewers. There are some exceptions: (1) simple PRs to some tools or tests; (2) situation when it's impossible to find another competent reviewer. In any case a PR should be reviewed by at least one reviewer
- If somebody left a comment in PR the author should wait for their approval

## Doing good code review:

- All comments in PR should be polite and concern the PR
- If a reviewer criticizes a PR they should suggest a better solution
- Reviewer's solution may be rejected by the author if the review did not give arguments why it should be done
- Comments in PR should be not only about things which are worth redeveloping but about good designs as well
- It's better to ask the developer to make code simpler or add comments to the code base than to understand the code through the developer's explanation
- It's worth writing comments in PR which help the PR author to learn something new
- All code base should conform to ./docs/CPP_STYLE.md, ./docs/OBJC_STYLE.md, ./docs/JAVA_STYLE.md or other style in ./docs/ depending on the language
- Code review should be started as quick as possible after a PR is published. A reviewer should answer developer comments as quick as possible
- If a PR looks good to a reviewer they should approve this PR. It means the review is finished
- If a developer changes the PR significantly, the reviewers who have already approved the PR should be informed about these changes
- A reviewer should pay attention not only to the code base but also to the description of the PR and commits
- A PR may be merged by a reviewer if all the following items are fulfilled: (1) the PR isn't marked as a draft; (2) all reviewers which have started to review the PR, approved it; (3) all reviewers which are added as reviewers of the PR, have approved it
- If a reviewer doesn't have time to review all the PR they should write about it explicitly. For example, LGTM for android part
- If a reviewer and a developer cannot find a compromise, the third opinion about it should be asked
- All comments about blank lines should be considered as optional

## Recommendations:

- Functions and methods should not be long. In most cases, it's good if the whole body of a function or method fits on the monitor. It's good to write a function or a method shorter than 60 lines
- Source files should not be very long
- If you are solving a big task it's worth splitting it into subtasks and develop one or several PRs for every subtask.
- If it's necessary to make a big change list which should be merged to master at once, it's worth creating a branch and make PRs on it. Then to make PR with all commits of the branch to the master
- In most cases refactoring should be done in a separate PR
- If you want to make a refactoring which touches a significant part of the code base, it's worth discussing it with all developers before starting changing code
- It's worth using Resolve conversation button to minimize the list of comments of a PR
