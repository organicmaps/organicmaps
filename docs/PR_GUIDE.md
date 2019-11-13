# Pull Request Guide

## General development rules:

- All the changes of the code base are implemented through pull requests (PR) on master or other branches
- When all the features for an upcoming release are implemented and are stable enough, a new release branch 
  is made from master. After that, all app versions are updated in the master branch in files:
  ./omim/xcode/common.xcconfig and ./omim/android/gradle.properties
- Only PR with bugfixes should be made on release branches. If it’s necessary it’s permitted to cherry-pick such
  commits to master
- If a commit is made on a release branch and it is not intended to be merged to master the text of the commit
  should have prefix [RELEASE ONLY]
- When a release is published, one of developers makes a PR with all new commits of the release to master
- All the further commits to release branch should be cherry-picked to the master by commits author
- If commits are mistakenly merged into master or release branch, a PR with new commits which revert them should be made
- When a PR to any branch is ready to merge, rebase and merge option should be used to make the code source tree
  flatter


## Writing good PR:

- PR should be small and reflect only one idea, a feature, a bugfix, or a refactoring. In most cases a PR of 100 lines
  or less is considered as a PR of normal size and a PR of 1000 lines as big one
- If a PR implements two different features, refactoring, or bugfixes, it should be split into several PRs
- The description field of every PR should contain a description, links to a ticket or/and links to the confluence
- If a PR implements a complex math task, the description should contain a link to the proof of the solution or a link
  to a trusted source
- All PRs should contain links to the corresponding tickets. The exception is refactoring if there’s nothing to test
  after it. The reason for that is that every feature or bugfix should be tested by testers after it’s implemented
- Every commit of all PRs should be compilable under all platforms. All tests should pass. So if changing of code
  breaks unit or integration tests these tests should be fixed in the same commit
- Every commit should reflect a completed idea and have an understandable comment. Review fixes should be merged into
  one commit
- New functionality and unit or integration tests for it should be developed in the same PR
- When some source files are changed and then some other source files based on them are auto generated, they should be
  committed in different commits. For example, one commit changes strings.txt and another one contains generated files
- Most commits and PRs should have prefixes in square brackets depending on the changed subsystem. For example,
  [routing], [generator], or [android]. Commits and PRs may have several prefixes
- If a PR should not be merged after review, the caption of it should be started with [DNM] prefix or the PR should be
  published as a draft. DNM means do not merge
- All commits and PR captions should be written in English. PR description and PR comments may be written in other
  languages
- All PRs should be approved by at least two reviewers. There are some exceptions: (1) simple PRs to some tools or
  tests; (2) situation when it’s impossible to find another competent reviewer. In any case a PR should be reviewed by
  at least one reviewer
- An author should not merge their PR. One of the reviewers should merge it
- If somebody left a comment in PR the author should wait for their approval


## Doing good code review:

- All comments in PR should be polite and concern the PR
- If a reviewer criticizes a PR they should suggest a better solution
- Reviewer's solution may be rejected by the author if the review did not give arguments why it should be done 
- Comments in PR should be not only about things which are worth redeveloping but about good designs as well
- It’s better to ask developer to make code simpler or add comments to code base than to understand the code through
  developer's explanation
- It’s worth writing comments in PR which help the PR author to learn something new
- All code base should conform to ./omim/docs/CPP_STYLE.md, ./omim/docs/OBJC_STYLE.md or other style in ./omim/docs/
  depending on the language
- Code review should be started as quick as possible after a PR is published. A reviewer should answer developer
  comments as quick as possible
- If a PR looks good to a reviewer they should approve this PR. It means the review is finished
- If a developer changes the PR significantly, the reviewers who have already approved the PR should be informed
  about these changes
- A reviewer should pay attention not only to code base but also to the description of the PR and commits
- A PR may be merged by a reviewer if all the following items are fulfilled: (1) the PR isn't marked as a draft or
  [DNM]; (2) all reviewers which have started to review the PR, approved it; (3) all reviewers which are added as
  reviewers of the PR, have approved it
- If a reviewer doesn’t have time to review all the PR they should write about it explicitly. For example, LGTM for
  android part
- If a reviewer and a developer cannot find a compromise, the third opinion about it should be asked
- If a reviewer is off but a PR should be merged urgently, the following steps should be done by the author: (1) try to
  find the reviewer and ask to finish the review; (2) ask CIO for a "gold" LGTM which let merge the PR without waiting
  for reviewers
- All comments about blank lines should be considered as optional


## Recommendations:

- Functions and methods should not be long. In most cases it’s good if the whole body of a function or method fits on
  the monitor. It’s good to write function or method shorter than 60 lines
- Source files should not be very long
- If you are solving a big task it’s worth splitting it into subtasks, then making in jira an epic for the big task and
  tickets in the epic for subtasks. Then develop one or several PRs for every task
- If it’s necessary to make a big change list which should be merged to master at once, it’s worth creating a branch and
  make PRs on it. Then to make PR with all commits of the branch to the master
- To build a PR and run tests it’s worth using a corresponding command from
  https://confluence.mail.ru/pages/viewpage.action?pageId=159831803
- In most cases refactoring should be done in a separate PR
- If you want to make a refactoring which touches a significant part of the code base, it’s worth discussing it with
  all developers before starting changing code
- It's worth using Resolve conversation button to minimize the list of comments of a PR
