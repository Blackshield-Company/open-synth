# Open Synth — Agent Brief

Open-source digital instrument (JUCE 8 + C++20, public on `master`): the "open-source Roland Juno-Di" — REAL instrument sounds (real piano, real drumkit), not synthesized approximations. CI green on 3 OS, release pipeline works. Samples are git submodules — do NOT restructure them. Existing codebase: READ IT before writing anything. Match existing conventions.

## Your mission: playability milestone
- [ ] Build the project first (`cmake` per README) and report actual state before changing anything
- [ ] Instrument browser polish: fast category switching (Piano/Organ/Drums/Bass/Pads), visible selected-instrument state, keyboard navigable
- [ ] Fix any papercuts the build surfaces
- [ ] Do NOT touch the release CI, sample submodule layout, or licensing

## Stack
JUCE 8 + C++20, CMake. UI direction: synthwave '84 is the brand here (this is the ONE project where it's the default, not opt-in).

## House Rules (non-negotiable)

- **Theme:** Blackshield (steel+blood: bg #101014, surface #16161C, text #D8D3C8, accent #C1121F) is the DEFAULT everywhere. Other palettes (incl. Synthwave '84) stay opt-in/selectable. See the blackshield-theme skill for the full token set.
- **CLI naming:** the binary is the bare project name. Never a `-cli` suffix.
- **Commits:** conventional commits (`feat:`, `fix:`, `chore:`...). Local commits are fine.
- **NEVER:** push to a remote, create GitHub remotes, force-push, rewrite/delete tags, or touch `.env`/credential files.
- **Done means verified:** build it AND run it before claiming completion. No stubs-as-deliverables.
