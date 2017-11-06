# Seminator
Seminator is a free tool for semi-determinization of omega-automata.

## Installation
Seminator depends on the [Spot](https://spot.lrde.epita.fr/index.html) library. You need a version 2.4 or higher installed in your system before compiling Seminator.

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

Jupyter notebook [Experimental evaluation](Experimental_evaluation.ipynb)
will guide you through the steps we used to generete the evaluation presented there. 
It compares Seminator to `ltl2ldba` and `nba2ldba` from the 
[Owl library](https://www7.in.tum.de/~sickert/projects/owl/).

However, both Owl and Seminator evolved since the paper was published
so new in this version of scripts you will find comparison with the
current version of Seminator and a recent version of Owl.

If the preview of the notebooks on gitHub does not work, use 
[Jupyter nbviewer](https://nbviewer.jupyter.org/github/mklokocka/seminator/blob/master/Experimental_evaluation.ipynb) instead

### Requirements

If you would like to run the notebook by yourself, you need to have the 
folowing tools installed in `PATH` on your system.

* [SPOT](https://spot.lrde.epita.fr/) v. 2.4+ with Python bindings
* [Pandas](http://pandas.pydata.org/) Python library v. 0.21.0+
* [Jupyter](http://jupyter.org/) notebook v 5.0+
* [R](https://www.r-project.org/) with libraries `ggplot2` and `colorspace`