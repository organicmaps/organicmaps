# Developer's Certificate of Origin

## CLA vs DCO

This project uses **Developer's Certificate of Origin (DCO)** instead of **Contributor License Agreement (CLA)**. It is often easier to get started contributing under a DCO than a CLA.

When one is developing software for a company they need to have the company sign the Corporate CLA prior to submitting contributions. That means there is a step after the business decides to contribute where legal documents need to be signed and exchanged. Once this is done there are steps to associate people with those legal documents. All of this takes time. In some companies this process can take weeks or longer.

We wanted to make it simpler to contribute.

## What Is A DCO?

A DCO is lightweight way for a developer to certify that they wrote or otherwise have the right to submit code or documentation to a project. The way a developer does this is by adding a Signed-off-by line to a commit. When they do this they are agreeing to the DCO.

The full text of the DCO can be found at <https://developercertificate.org>. It reads:

> Developer Certificate of Origin Version 1.1

> Copyright (C) 2004, 2006 The Linux Foundation and its contributors. 1 Letterman Drive Suite D4700 San Francisco, CA, 94129

> Everyone is permitted to copy and distribute verbatim copies of this license document, but changing it is not allowed.

> Developer's Certificate of Origin 1.1

> By making a contribution to this project, I certify that:

> (a) The contribution was created in whole or in part by me and I have the right to submit it under the open source license indicated in the file; or

> (b) The contribution is based upon previous work that, to the best of my knowledge, is covered under an appropriate open source license and I have the right under that license to submit that work with modifications, whether created in whole or in part by me, under the same open source license (unless I am permitted to submit under a different license), as indicated in the file; or

> (c) The contribution was provided directly to me by some other person who certified (a), (b) or (c) and I have not modified it.

> (d) I understand and agree that this project and the contribution are public and that a record of the contribution (including all personal information I submit with it, including my sign-off) is maintained indefinitely and may be redistributed consistent with this project or the open source license(s) involved.

An example signed commit message might look like:

```
An example commit message

Signed-off-by: Some Developer <somedev@example.com>
```

Git has a flag that can sign a commit for you. An example using it is:

```
$ git commit -s -m 'An example commit message'
```

In the past, once someone wanted to contribute they needed to go through the CLA process first. Now they just need to signoff on the commit.

## Contributors

Please feel free to add your name to [CONTRIBUTORS](https://github.com/organicmaps/organicmaps/blob/master/CONTRIBUTORS) in your first PR.
