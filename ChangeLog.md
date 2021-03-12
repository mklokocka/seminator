## dev

### Changed

* Seminator now requires a C++17 compiler.

### Fixed

* `--preprocess=0 --via-tgba` did still preprocess the input automaton (thanks Tobias John for reporting this).

## [2.0] - 2020-01-25
The code was almost completely rewritten.

### Added
* New options control when cut-transitions (jumps to deterministic component) are created:
  - `--cut-highest-mark`: jump when a highest mark occurs in the input automaton (old default)
  - `--cut-on-SCC-entry`: jump also when freshly entering an accepting SCC
  - `--cut-always`: jump also for all transitions leading to some accepting SCC (new default)
* New options to control optimizations (all enabled by default):
  - `--powerset-on-cut` starts the breakpoint/subset construction already on cut-transitions. For a marked transition s -a-> p build s -a-> (δ(s),δ_0(s),0) instead of s -a-> ({p},∅,0).
  - `--powerset-for-weak` for accepting SCC that is inherently weak in the input automaton, use only simple powerset construction in the deterministic part of the sDBA.
  - `--bscc-avoid` in the 1st component, avoid deterministic SCCs with no nondeterministic successors and rather jump to the 2nd component directly.
  - `--reuse-deterministic` is similar to `--bscc-avoid`, but reuses the SCCs as they are, potentialy with TGBA acceptance.
  - `--skip-levels` enable level-skipping. For 1 edge we can "skip" multiple levels.
  - `--jump-to-bottommost` removes states of the form `s=(R,B,l)` if there is some other state (R,B',l') from which `s` cannot be reached
  - `--scc-aware` enables the SCC-aware optimization
  - `--pure` disable all optimization except `--scc-aware`
* Multiple transformation types (`--via-sba`, `--via-tba`, `--via-tgba`) can be specified. The one will smallest result will be output.
* `--complement[=best|spot|pldi]` complement the automaton after semi-determinizing it, using one of two NCSB-based complementation algorithms.
* Options to control pre- and post-processings:
  - `--preprocess` simplify the automaton with Spot before semi-determinizing it
  - `--postprocess` simplify the automaton with Spot after semi-determinizing it
  - `--postprocess-comp` simplify the automaton with Spot after complementing it
* `--highlight` colors states regarding the part of sDBA: violet for the 1st (nondeterministic) part, green for the 2nd (deterministic) part. Cut edges are colored by red.
* Seminator now understands `-h` for help.
* Seminator now detects more errors (unsupported options, failure to output, incompatible options, ...)
* Python bindings are now available as an extension to Spot's bindings:
  ```
  import spot.seminator as sem
  ```
  will import the following functions
  ```
  sem.semi_determinize()
  sem.complement_semidet()
  sem.highlight_components()
  sem.highlight_cut()
  sem.is_cut_deterministic()
  sem.seminator()
  ```

### Changed
* seminator now processes a stream of automata instead of only one at time.
* `--is-cd` now acts like a filter: instead of outputing 0 or 1, it prints the input automaton if it is cut-deterministic, or discard it otherwise (this behavior is more in line with `autfilt --is-semi-deterministic` from Spot)
* When a breakpoint is reached (R = B), track accepting transitions of the next level in B (it was set to ∅ before).
* `--version` now also prints version of Spot
* building process now relies on autotools (`./configure; make`)

### Removed
* `--cy` option (use `--via-tba --pure -scc-aware=0` instead)
* files from the experimental evaluation of version 1.2

### Fixed
* jumps to the deterministic component after seeing highest mark (instead of 0) also for cut-deterministic automata
* handling of stdin

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

[Unpublished]: https://github.com/mklokocka/seminator/compare/v2.0...HEAD
[2.0]: https://github.com/mklokocka/seminator/compare/v1.2.0...v2.0
[1.2.0]: https://github.com/mklokocka/seminator/compare/v1.1.0...v1.2.0
[1.1.0]: https://github.com/mklokocka/seminator/compare/v1.0.0...v1.1.0
