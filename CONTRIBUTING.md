# Contributing

When contributing to this project, please first discuss the change you wish to
make via issue, email, or any other method with the authors before
making a change.

Please note we have a code of conduct, please follow it in all your interactions
with the project.

If your contribution is an entirely new module (a new `:Block` and/or
`:Solver` repository), start it from the
[ModuleTemplate](https://gitlab.com/smspp-develop/moduletemplate) repository (in the
private `smspp-develop` group of the SMS++ developers): its
`init.sh` script generates a complete module in the standard SMS++ layout
(builds, CI, tests and boilerplate) and registers it in the umbrella project.

## Merge Request Process

1. Remove any build and temporary files before committing the changes.

2. Update the Changelog accordingly.
   The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

   Say in the commit itself what it changes, with the trailer

       Changelog: Added
       Changelog-entry: `MCFSolver::get_var_direction()`, which writes the
         cycle of negative cost that certifies the unboundedness of the flow

   where the first line is one of the sections of the format (`Added`,
   `Changed`, `Deprecated`, `Removed`, `Fixed`, `Security`) and the second
   is the entry as it will read in the `CHANGELOG.md`, carried on as many
   lines as it takes. The entry is written when the work is done and is
   understood, rather than reconstructed at the release out of a year of
   commits, and `changelog draft` collects the trailers of a release into
   the draft of its changelog.

   A commit that changes nothing worth telling — a rename of a local
   variable, a comment, the formatting of a file — says so with
   `[skip changelog]` anywhere in its message.

   The pipeline holds this: `changelog check` reads the `CHANGELOG.md` of
   the project and complains about what does not keep the format, and on a
   merge request `changelog coverage` asks for a trailer, or the marker, or
   an entry of `[Unreleased]` covering each of its commits. The tool is
   `changelog` in the umbrella, and works on one repository at a time with
   `--repo`.

   Besides the format, `check` asks that a released version say what it was
   released with and say it once: an entry that is also in another release,
   a section named twice in the same release, an entry sitting under no
   section at all and a formula between dollars, which Markdown does not
   build, are each a complaint. The form of the file — the blanks at the
   end of a line, the blank line between two entries, the case an entry of
   `[Unreleased]` opens with and the full stop it closes with — is not a
   complaint but a command: `changelog tidy -w` puts it as the project
   writes it.

3. The version is derived automatically from the most recent git tag (see
   `cmake/DeriveVersion.cmake`), so there is no version number to bump by hand:
   cutting a release is just tagging the release commit, e.g. `git tag x.y.z`.
   This project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

4. You may merge the Merge Request in once you have the sign-off of one 
   of the project maintainers, or if you do not have permission to do that,
   you may request the maintainer to merge it for you.

## Code of Conduct

### Our Pledge

In the interest of fostering an open and welcoming environment, we as
contributors and maintainers pledge to making participation in our project and
our community a harassment-free experience for everyone, regardless of age,
body size, disability, ethnicity, gender identity and expression, level of
experience, nationality, personal appearance, race, religion, or sexual identity
and orientation.

### Our Standards

Examples of behavior that contributes to creating a positive environment
include:

* Using welcoming and inclusive language
* Being respectful of differing viewpoints and experiences
* Gracefully accepting constructive criticism
* Focusing on what is best for the community
* Showing empathy towards other community members

Examples of unacceptable behavior by participants include:

* The use of sexualized language or imagery and unwelcome sexual attention or
  advances
* Trolling, insulting/derogatory comments, and personal or political attacks
* Public or private harassment
* Publishing others' private information, such as a physical or electronic
  address, without explicit permission
* Other conduct which could reasonably be considered inappropriate in a
  professional setting

### Our Responsibilities

Project maintainers are responsible for clarifying the standards of acceptable
behavior and are expected to take appropriate and fair corrective action in
response to any instances of unacceptable behavior.

Project maintainers have the right and responsibility to remove, edit, or
reject comments, commits, code, wiki edits, issues, and other contributions
that are not aligned to this Code of Conduct, or to ban temporarily or
permanently any contributor for other behaviors that they deem inappropriate,
threatening, offensive, or harmful.

### Scope

This Code of Conduct applies both within project spaces and in public spaces
when an individual is representing the project or its community. Examples of
representing a project or community include using an official project e-mail
address, posting via an official social media account, or acting as an appointed
representative at an online or offline event. Representation of a project may be
further defined and clarified by project maintainers.

### Enforcement

Instances of abusive, harassing, or otherwise unacceptable behavior may be
reported by contacting the project team.
All complaints will be reviewed and investigated and will result in a response
that is deemed necessary and appropriate to the circumstances. The project team
is obligated to maintain confidentiality with regard to the reporter of an
incident. Further details of specific enforcement policies may be posted
separately.

Project maintainers who do not follow or enforce the Code of Conduct in good
faith may face temporary or permanent repercussions as determined by other
members of the project's leadership.

### Attribution

This Code of Conduct is adapted from the [Contributor Covenant][homepage],
version 1.4, available at [http://contributor-covenant.org/version/1/4][version]

[homepage]: http://contributor-covenant.org
[version]: http://contributor-covenant.org/version/1/4/
