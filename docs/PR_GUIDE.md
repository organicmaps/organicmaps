# Pull Request Guide

## General development rules:

- All the changes of the code base are implemented through pull requests (PR) on master or other branches
- When all next release features are implemented in master and master is stable enough release branch is made from it
- Only PR with bugfix should be done on release. If it’s necessary it’s permitted to cherry-pick such bugfix PR to master
- When a release is published one of developers makes a PR with all new commits of the release to master
- All the further commits to release branch should be cherry-picked to the master by commits author
- When a PR to any branch is about to merge, rebase and merge option should be used to make the code source tree flatter.


## Writing good PR:

- PR should be small and reflect only one idea, a feature, a bugfix or a refactoring. In most cases a PR of 100 lines or less is considered as a PR of normal size and a PR of 1000 lines as big one
- If a PR implements two different features, refactoring or bugfixes it should be to split into several PRs
- A description field of every PR should contain a description, links to a ticket or/and links to article to confluence.
- If a PR implements a complex math task the description should contain a link to the proof of the solution or a link to a trusted source
- All PRs should contain a corresponding ticket in jira. The exception is refactoring if there’s nothing to test after it. The reason for that is that every feature or bugfix should be tested by testers after it’s implemented
- Every commit of all PR should be compiled under all platforms. All tests should pass. So if changing of code breaks unit or integration tests these tests should be fixed in the same commit.
- New functionality and unit or integration tests on it should be developed in the same PR
- When some source files are changed and then some other source files based on it auto generated, they should be committed in different commits. For example, one commit changes strings.txt and another one files which generated based on it
- Most commits and PRs should have prefixes in square brackets depending on a changed subsystem. For example, [routing], [generator] or [android]. A commits and a PR may have several prefixes
- If a PR should not be merge after review the caption of it should be started with [DNM] prefix or the PR should be publish as a draft. DNM means do not merge
- All commits and a PR captions should be written in english. PR description and PR comments may be written in other languages
- All PR should be approved by at least two reviewers. There’re some exceptions: (1) simple PRs to some tools or tests; (2) situation when it’s impossible to find another competent reviewer. In any case a PR should be reviewed by at least one reviewer
- An author should not merge its own PR. One of reviewer should merge it
- If somebody left a comment in PR the author should be wait for his or her review


## Doing good code review:

- All comments in PR should be polite and concern the PR
- If a reviewer criticizes a PR he or she should suggest a better solution
- Comments in PR should be not only about things which are worth redeveloping but about good designs as well
- It’s better to ask developer to make code simpler then or add comments to code base then to understand the code through developer's explanation
- It’s worth writing comments in PR which help a PR author to study something new
- All code base should conform ./omim/docs/CPP_STYLE.md
- Code review should be started as quick as possible after a PR is published. A reviewer should answer developer comments as quick as possible
- If a PR looks good to a reviewer he or she should write LGTM as a comment of this PR. It means the review is finished
- If a developer changes his or her PR significantly he or she should inform reviewers, who put LGTM about these changes
- A reviewer should pay his attention not only to code base but also to the description of PR and commits
- If a reviewer don’t have time to review all the PR he or she should write about it explicitly. For example, LGTM for android part
- If a reviewer and a developer cannot find a compromise the third opinion about it should be asked
- If a reviewer is off but a PR should be merge urgently the following steps should be done by author: (1) try to find the reviewer and ask to finish the review; (2) ask CIO for a «gold» LGTM which let merge the PR without waiting for reviewers


## Recommendations:

- Functions and method should not be long. In most cases it’’s good if whole body of a function or method it’s possible to see on the monitor. It’s good to write function or method shorter than 60 lines
- Source files should not be very long
- If you solve a big task it’s worth splitting it into subtasks, then making in jira an epic for the big task and tickets in the epic for subtasks. Then developing one or several PRs for every task
- If it’s necessary to make a big change list which should be merge to master at once, it’s worth creating a branch and make PRs on it. Then to make PR with all commits of the branch to the master
- To build a PR and run tests it’s worth using a corresponding command from https://confluence.mail.ru/pages/viewpage.action?pageId=159831803
- In most cases refactoring should be done in a separate PR
- If you want to make a refactoring which touches a significant part of the code base it’s worth discussing it with all developers before to start changing code
