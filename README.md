# cmpe146-2026-MSP-EXP432P401R

Public template repo for students taking my CmpE 146 course in 2026 using the
MSP-EXP432P401R.

## Getting your workspace set up

This repo holds a Code Composer Studio (Theia) project per lab. It ships its
own workspace file, `cmpe146_msp_exp432p401r.theia-workspace`, scoped to just
this repo's folder:

1. Follow [`docs/code_composer_studio_setup_instructions.pdf`](docs/code_composer_studio_setup_instructions.pdf)
   to install CCS along with the **SimpleLink MSP432 SDK** (3.40.1.02) - this
   is what lets CCS resolve this repo's `${COM_TI_SIMPLELINK_MSP432_SDK_*}`
   build variables.
2. Open CCS Theia and use **File > Open Workspace...** (or the equivalent in
   the Explorer overflow menu) to open
   `cmpe146_msp_exp432p401r.theia-workspace` from the folder you cloned this
   repo into.

CCS Theia only allows one project of a given name per open workspace. Every
lab in this repo is named `labN_*` to match the same lab in the sibling
[cmpe146-2026-MSPM0G3507](https://github.com/kammce/cmpe146-2026-MSPM0G3507)
repo, so if you add this repo's folder into a workspace that already has that
repo's folder in it (e.g. via "Add Folder to Workspace" on a parent
directory), CCS will refuse to load whichever lab loads second because the
name collides. Opening this repo's own `.theia-workspace` file instead avoids
that entirely, since it scopes the session to only this repo's projects. Note
that CCS Theia only supports one open workspace at a time - switch workspace
files when you move between this repo and the MSPM0G3507 one.

## Repository layout

```text
cmpe146-2026-MSP-EXP432P401R/
├── hal/                        Shared hardware abstraction interfaces (pure
│                               virtual classes) - one header per peripheral
│                               category, used across all labs. Given, don't
│                               modify.
├── datasheets/                 MSP432P4xx TRM, datasheet, and LaunchPad
│                               user guide PDFs.
├── docs/                       CCS setup instructions and other non-chip
│                               reference docs.
├── lab1_gpio/                  Lab 1: GPIO driver, from scratch.
├── style.md                    This codebase's C++ style guide.
└── .clang-format, .clang-tidy,  Formatting, lint, and spell-check config
    .cspell.json,                used by this repo's pre-commit hooks and
    .pre-commit-config.yaml,     CI (.github/workflows/lint.yml).
    .github/
```

Each `labN_*/` folder is a self-contained CCS project:

- `.project` / `.cproject` / `.ccsproject` / `.settings/` - CCS project
  metadata, generated/maintained by CCS, don't hand-edit.
- `targetConfigs/` - debug probe + device configuration.
- `labN_*.cpp` - where you write your driver classes and the lab's `main()`,
  against the shared interfaces in `../hal/`.
- `README.md` - that lab's assignment and grading rubric.
