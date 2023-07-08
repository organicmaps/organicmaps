# How to write a commit message

Any commit needs a helpful message. Mind the following guidelines when committing to any of Organic Maps repositories at GitHub.

1. Separate subject from body with a blank line.
2. Limit the subject line to **72 characters**.
3. Prefix the subject line with a **subsystem name** in square brackets:

   - [android]
   - [ios]
   - [qt]
   - [search]
   - [generator]
   - [strings]
   - [platform]
   - [storage]
   - [transit]
   - [routing]
   - [bookmarks]
   - [3party]
   - [docs]
   - ...
   - See `git log --oneline|egrep -o '\[[0-9a-z]*\]'|sort|uniq -c|sort -nr|less` for ideas.

4. Start a sentence with a capital letter.
5. Do not end the subject line with a period.
6. Do not put "gh-xx", "closes #xxx" to the subject line.
7. Use the imperative mood in the subject line.

   - A properly formed Git commit subject line should always be able to complete
     the following sentence: "If applied, this commit will _/your subject line here/_".

8. Wrap the body to **72 characters** or so.
9. Use the body to explain **what and why** vs. how.
10. Link GitHub issues on the last lines:

    - [See tutorial](https://help.github.com/articles/closing-issues-via-commit-messages).

11. Use your real name and real email address.

    - See also [Developer's Certificate of Origin](DCO.md)

A template:

```
    [subsystem] Summarize in 72 characters or less

    More detailed explanatory text, if necessary.
    Wrap it to 72 characters or so.
    In some contexts, the first line is treated as the subject of the
    commit, and the rest of the text as the body.
    The blank line separating the summary from the body is critical
    (unless you omit the body entirely); various tools like `log`,
    `shortlog` and `rebase` can get confused if you run the two together.

    Explain the problem that this commit is solving. Focus on why you
    are making this change as opposed to how (the code explains that).
    Are there side effects or other unintuitive consequences of this
    change? Here's the place to explain them.

    Further paragraphs come after blank lines.

    - Bullet points are okay, too.

    - Typically a hyphen or asterisk is used for the bullet, preceded
      by a single space, with blank lines in between, but conventions
      vary here.

    Fixes: #123
    Closes: #456
    Needed for: #859
    See also: #343, #789
```

Based on [Tarantool Guidelines](https://www.tarantool.io/en/doc/latest/dev_guide/developer_guidelines/#how-to-write-a-commit-message).
