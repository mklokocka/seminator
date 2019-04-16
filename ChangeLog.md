## [Unpublished]
The code was almost completely rewritten.
### Added
* New options that control when cut-transitions (jumps to deterministic component) are created. By default a cut-transition is created when a highest mark occurs in the input automaton.
  - `--cut-on-SCC-entry`: jump also when freshly entering an accepting SCC
  - `--cut-always`: jump also for all transitions leading to some accepting SCC
* `--powerset-on-cut` starts the breakpoint/subset construction already on cut-transitions. For a marked transition s -a-> p build s -a-> (δ(s),δ_0(s),0) instead of s -a-> ({p},∅,0).
* `--powerset-for-weak` for accepting SCC that is inherently weak in the input automaton, use only simple powerset construction in the deterministic part of the sDBA.
* Multiple transformation types (`--via-sba`, `--via-tba`, `--via-tgba`) can be specified. The one will smallest result will be outputted.
* `--scc-aware` enables the scc-aware optimization (default on)
* `--scc0` and `--no-scc-aware` disables the SCC-aware optimization
* `--sd` option for semi-deterministic output (and not cut-deterministic)
* `--no-reduction` is a synonym for `s0` (disables reductions by Spot)
* `--highlight` colors states regarding the part of sDBA: violet for the 1st (nondeterministic) part, green for the 2nd (deterministic) part. Cut edges are colored by red.
* Seminator now understands `-h` for help.
* Seminator now detects unsupported options (exits with return status 2)

### Changed
* When a breakpoint is reached (R = B), track accepting transitions of the next level in B (it was set to ∅ before).
* `--version` now also prints version of Spot

#### Evaluation
* Use the new version of owl for comparison (18.06)
* Also compare Seminator with the `--cut-on-SCC-entry` option
* Also compare Seminator with the `--cut-always` option

### Removed
* `--cy` option (use --via-tba -s0 instead)

### Fixed
* jumps to the deterministic component after seeing highest mark (instead of 0) also for cut-deterministic automata
* Handling of stdin; thanks Alexandre Duret-Lutz for the fix.

## [1.2.0] - 2017-11-06
### Added
* SCC-aware optimization: States of the deterministic component are constraint to follow states from one SCC of the input automaton only.
* Scatter plots in pdf are now available in the repo.

### Changed
* Jump to the deterministic component to level 0 after reading mark with the highest number (previously after mark 0).
* Evaluation now uses Pandas to create the tables.
* Evaluation now uses Owl v. 1.1.0 for comparison.
* Seminator now compiles with C++14 standard.

## [1.1.0] - 2017-05-02
### Added
* Check for automata cut-determinism
* `--via_tgba`, `--via_tba`, and `--via_sba` options for different pipelines of translations:
  - `--via_tgba`: TGBA -> sdBA (default)
  - `--via_tba`: TGBA -> TBA -> sdBA
  - `--via_sba`: TGBA -> SBA -> sdBA
* Use all three ways and choose the best result.
### Changed
* Use Spot's simplifications by default.
* Use `-s0` to disable Spot's simplifications instead of `-o` to enable them.

## 1.0.0 - 2017-01-25
### Added
* Seminator translates input transition-based generalized Büchi automaton into a semi-deterministic or cut-deterministic Büchi automaton.

[Unpublished]: https://github.com/mklokocka/seminator/compare/v1.2.0...HEAD
[1.2.0]: https://github.com/mklokocka/seminator/compare/v1.1.0...v1.2.0
[1.1.0]: https://github.com/mklokocka/seminator/compare/v1.0.0...v1.1.0
