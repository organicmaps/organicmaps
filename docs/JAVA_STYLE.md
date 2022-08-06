# Java Style Guide

The Android app follows the style defined in `android/code_style_scheme.xml`. Code not respecting this style will not be accepted and you will be asked by reviewers to edit your PR.

## Importing the style in Android Studio

You can import this configuration file in Android Studio to allow automatically formatting Java files. When Android Studio is open, go to `File -> Settings` then open the section `Editor -> Code Style -> Java`. Here click on the gear icon next to the dropdown at the top of the page and select `Import Scheme...`. Select `android/code_style_scheme.xml` and press `OK` in the next dialog.

## Formatting a file

Before making a commit, please reformat your files using `Code -> Reformat File`. In the opening dialog, select the scope `Only VCS changed text` and tick all the optional checkbox (optimize imports, rearrange code and code cleanup).
