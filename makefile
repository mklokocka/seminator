## Copyright (C) 2017, Fakulta Informatiky Masarykovy univerzity
##
## This file is part of Seminator, a tool for semi-determinization of omega automata.
##
## Seminator is free software; you can redistribute it and/or modify it
## under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 3 of the License, or
## (at your option) any later version.
##
## Seminator is distributed in the hope that it will be useful, but WITHOUT
## ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
## or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
## License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.

CC=gcc
CXX=g++
RM=rm -f
CPPFLAGS=-std=c++14 -g -I/usr/local/include
LDFLAGS=-std=c++14 -g -lstdc++
LDLIBS=-L/usr/local/lib -lspot -lbddx

SRCS=seminator.cpp
OBJS=$(SRCS:.cpp=.o)

all: seminator

seminator: $(OBJS) ; $(CXX) $(LDFLAGS) -o seminator $(OBJS) $(LDLIBS)

seminator.o: seminator.cpp

clean: ; $(RM) $(OBJS)

distclean: clean ; $(RM) seminator

.PHONY: all clean distclean
