# Seminator
Seminator is a free tool for semi-determinization of omega-automata.

## Installation
Seminator depends on the [Spot](https://spot.lrde.epita.fr/index.html) library. You need a version 2.3.2 or higher installed in your system before compiling Seminator.

After Spot is installed you can use the attached `makefile` to compile Seminator.

```
make
```

## Basic usage
The following command translates a possibly nondeterministic transition-based generalized Büchi automaton (TGBA) placed in file `aut.hoa` into a semi-detetrministic Büchi automaton (SDBA) and prints the result int the [HOA format](https://adl.github.io/hoaf/) to the standard output.
```
./seminator -f aut.hoa
```

For more information run:
```
./seminator --help
```

## Experimental evaluation
Seminator was presented in the paper _František Blahoudek, Alexandre Duret-Lutz, Mikuláš Klokočka, Mojmir Kretinsky and Jan Strejcek. **Seminator: A Tool for Semi-Determinization of Omega-Automata.** In Proceedings of [LPAR-21](http://easychair.org/smart-program/LPAR-21/LPAR-index.html), 2017._

To reproduce the experimental evaluation done for this paper, install [Jupyter notebook](https://jupyter.org) and then follow the [Experimental evaluation](Experimental_evaluation.ipynb) notebook. You will also need to have a working installation of [R](https://www.r-project.org/).
