# Seminator
Seminator is a free tool for semi-determinization and complementation of omega-automata. 
[![Sandbox][badge sandbox]][sandbox seminator]

Seminator 2 was presented at [CAV'2020] and is described in the paper
*[Seminator 2 can complement generalized Büchi automata via improved semi-determinization][paper]*.
 The [presentation][talk] of the paper is available on YouTube 
 [![Talk][badge youtube]][talk] and you can also watch the [teaser] for the presentation.

## Installation

Seminator depends on the [Spot] library. You need version 2.8.4 or higher 
installed in your system before compiling Seminator.  
Also, unless Python bindings are disabled (with `./configure --disable-python`), 
you will need Python 3 development files installed on your system.
If you would like to run the notebooks or the test-suite, you need [Jupyter] notebook v 5.0+.

After Spot is installed you can run `./configure && make` to compile Seminator.

You may use `./configure --prefix=PREFIXDIR && make && sudo make install` to 
install Seminator in the same place as Spot.  If you need to install Seminator 
in a different location, use `./configure --prefix=OTHERDIR --with-spot=PREFIXDIR`,
replacing `PREFIXDIR` by the directory that was passed to Spot's `--prefix` 
configuration option.

On Linux, if you installed Seminator with `--prefix=/usr/local` (the default) or 
`--prefix=/usr`, make sure you run `sudo ldconfig` after `sudo make install` so 
that the shared library can be found.  If you installed Seminator in another 
directory, you probably need to set the `LD_LIBRARY_PATH` environment variable 
to the directory where the shared library was installed.

### Installation from git
You might need to run `autoreconf -i` before calling `./configure && make`.

## Try it out

The [SPOT's sandbox][sandbox spot] comes with Seminator 2 pre-installed and also
contains the notebooks from the [notebooks directory](notebooks).
[![Sandbox][badge sandbox]][sandbox seminator]

## Basic usage
The following command translates a possibly nondeterministic transition-based
generalized Büchi automaton (TGBA) placed in file `aut.hoa` into a 
semi-deterministic Büchi automaton (SDBA) and prints the result in the
[HOA format][hoa] to the standard output.
```
./seminator aut.hoa
```

The following command complements the TGBA placed in file `aut.hoa` using semi-determinization as an intermediate step.
```
./seminator --complement aut.hoa
```

For more information run:
```
./seminator --help
```

The following picture gives a summary of the different options and how they affect the tool.
![workflow](workflow.svg)

## Python bindings

For example of using the Python bindings, see the notebooks in the `notebooks/`
directory.  [![nbviewer][badge nbviewer]](https://nbviewer.jupyter.org/github/mklokocka/seminator/tree/next/notebooks/)
[![Sandbox][badge sandbox]][sandbox seminator]

Unless you have run `make install` to install the `seminator` binary and all the
Python bindings, is using the bindings require a bit of setup to make sure that
Python finds the correct files.  The easiest way to use the non-installed bindings
is use the `tests/pyrun` script: use `tests/pyrun python3` or 
tests/pyrun jupyter notebook` for interactive uses, or `tests/pyrun mycript.py`
for an actual script.

## Experimental evaluation

The experimental evaluation for version 2.0 is now kept in a 
[separate repository][evaluation] and is accessible in a Docker image, see below.

The evaluation of the previous versions, check out the source of this repository
for [version 1.2](https://github.com/mklokocka/seminator/tree/v1.2.0) (or older).

## Docker image with pre-installed tools

A Docker image containing a compiled version of Seminator, several related tools 
readily usable to reproduce the experimental-evaluation or simply experiment 
with the tools can be found [on DockerHub](https://hub.docker.com/r/gadl/seminator).
[![Binder][badge binder]](https://mybinder.org/v2/gh/adl/seminator-docker/master)

## Citing

If you need to reference Seminator in an academic publication, please cite the following paper.

**Seminator 2 can complement generalized Büchi automata via improved semi-determinization.**
_František Blahoudek, Alexandre Duret-Lutz, and Jan Strejček._
In Proceedings of the 32nd International Conference on Computer-Aided Verification (CAV'20),
volume 12225 of Lecture Notes in Computer Science, pages 15–27. Springer, July 2020.
DOI: [10.1007/978-3-030-53291-8_2]
[[bib], [pdf]] 

[10.1007/978-3-030-53291-8_2]: http://dx.doi.org/10.1007/978-3-030-53291-8_2
[bib]: blahoudek.20.cav.bib
[paper]: https://link.springer.com/chapter/10.1007%2F978-3-030-53291-8_2
[pdf]: https://www.lrde.epita.fr/~adl/dl/adl/blahoudek.20.cav.pdf

### Outdated reference
Version 1.1 of Seminator was presented in the following paper.

**Seminator: A Tool for Semi-Determinization of Omega-Automata.**
_František Blahoudek, Alexandre Duret-Lutz, Mikuláš Klokočka, Mojmír Křetínský and Jan Strejček._
Proceedings of the 21th International Conference on Logic for Programming,
Artificial Intelligence, and Reasoning (LPAR-21),
volume 46 of EPiC Series in Computing, pages 356–367. EasyChair Publications, May 2017.
DOI: [10.29007/k5nl](http://dx.doi.org/10.29007/k5nl)
[[bib](blahoudek.17.lpar.bib), [pdf](https://www.lrde.epita.fr/~adl/dl/adl/blahoudek.17.lpar.pdf)] 

[CAV'2020]: http://i-cav.org/2020/
[talk]: https://www.youtube.com/watch?v=3ocgIrdXo9A&list=PLMPy362FkW9qqkvgT66jr60OBm-2WtJZ-&index=15&t=4182s
[teaser]: https://www.lrde.epita.fr/~adl/dl/adl/blahoudek.20.cav.teaser.mp4

[evaluation]: https://github.com/xblahoud/seminator-evaluation
[hoa]: https://adl.github.io/hoaf
[Jupyter]: http://jupyter.org
[Spot]: https://spot.lrde.epita.fr

[badge binder]: https://mybinder.org/badge_logo.svg
[badge nbviewer]: https://raw.githubusercontent.com/jupyter/design/master/logos/Badges/nbviewer_badge.svg?sanitize=true
[badge sandbox]: https://img.shields.io/badge/Try%20online-Spot--sandbox-00ADee
[badge youtube]: https://img.shields.io/badge/watch-YouTube-red

[sandbox spot]: https://spot-sandbox.lrde.epita.fr
[sandbox seminator]: https://spot-sandbox.lrde.epita.fr/tree/seminator%20%28read%20only%29