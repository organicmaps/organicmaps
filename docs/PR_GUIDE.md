# Pull Request Guide

## Writing a good Pull Request (PR):

- If you used LLM tools (e.g. GitHub Copilot, ChatGPT, etc.) to generate code you should mention it explicitly in the PR description
- Format the code as described in ./docs/CODE_STYLE_GUIDE.md
- PR should be small and reflect only one idea, a feature, a bugfix, or a refactoring. In most cases, PR of 100 lines or less is considered as PR of normal size and PR of 1000 lines as big one
- If PR implements different/unrelated features, refactoring, or bugfixes, it should be split into several PRs
- The description field of every PR should contain a description, links to the related issue (or `Fixes #issue_number`), information about testing and screenshots/video if applicable
- If PR implements a complex algorithm, the description should contain a link to the proof of the solution or a link to a trusted source
- Every commit of PR should be compilable under all platforms. All tests should pass. So if changing of code breaks unit or integration tests these tests should be fixed in the same commit
- Every commit should reflect a completed idea and have an understandable comment
- Review fixes can be done in separate commits to ease the review process, but they should be squashed before merging into logical commits
- New functionality and unit or integration tests for it should be developed in the same PR
- There are no testers in the project. So every PR should contain information about how the changes were tested. A good testing involves different OS versions, landscape/portrait modes, light/dark themes if applicable, different use cases, etc.
- Automatically generated files (e.g. strings or styles) should always be in a separate commit
- Most commits and PRs should have prefixes in square brackets depending on the changed subsystem. For example, [routing], [generator], or [android]. Commits and PRs may have several prefixes
- PR which should not be merged after review should be marked as a draft
- All commits and PR captions should be written in English. PR description and PR comments may be written in other languages, although English is preferred

## Doing good code review:

- All comments in PR should focus on the code and the task, not on the developer
- If a reviewer criticizes PR they should suggest a better solution
- Reviewer's solution may be rejected by the author if the review did not give arguments why it should be done
- Comments in PR should be not only about things which are worth redeveloping but about good designs as well
- It's better to ask the developer to make code simpler or add comments to the code base than to understand the code through the developer's explanation
- It's worth writing comments in PR which help the PR author to learn something new
- Code review should be started as quick as possible after PR is published. A reviewer should answer developer comments as quick as possible
- If PR looks good to a reviewer they should approve that PR
- If a developer changes the PR significantly, the reviewers who have already approved the PR should be informed about these changes
- A reviewer should pay attention not only to the code base but also to the description of the PR and commits
- PR may be merged by a reviewer if all the following items are fulfilled: (1) the PR isn't marked as a draft; (2) all reviewers which have started to review the PR, approved it; (3) all reviewers which are added as reviewers of the PR, have approved it
- If a reviewer doesn't have time to review all the PR they should write about it explicitly. For example, LGTM for android part
- If a reviewer and a developer cannot find a compromise, a third opinion should be sought

## Recommendations:

- Clarifying the task before starting work may save time for both developers and reviewers
- Timely response to review comments speeds up the development process
- Functions and methods should not be long. In most cases, it's good if the whole body of a function or method fits on the monitor. It's good to write a function or a method shorter than 60 lines
- Source files should not be very long
- If you are solving a big task it's worth splitting it into subtasks and develop one or several PRs for every subtask
- In most cases refactoring should be done in a separate PR
- If you want to refactor a significant part of the code base, it's worth discussing it with all developers before starting work
- It's worth using the 'Resolve' conversation button to minimize the list of comments in PR
