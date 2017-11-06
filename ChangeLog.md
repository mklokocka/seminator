## [Unpublished]

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