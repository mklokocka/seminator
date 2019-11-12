#!/bin/sh

# Run tests on random automata
exec ltlcross -D -F ../formulae/random_sd.ltl -F ../formulae/random_nd.ltl \
         --stop-on-error \
         --reference ltl2tgba \
         'ltl2tgba -D %f | ../seminator -s0 --cd > %O' \
         'ltl2tgba -D %f | ../seminator -s0 > %O' \
         'ltl2tgba -D %f | ../seminator -s0 --powerset-for-weak > %O' \
         'ltl2tgba -D %f | ../seminator -s0 --cd --cut-on-SCC-entry > %O' \
         'ltl2tgba -D %f | ../seminator -s0 --cut-always > %O' \
         'ltl2tgba -D %f | ../seminator -s0 --cut-on-SCC-entry --powerset-for-weak > %O' \
         'ltl2tgba -D %f | ../seminator -s0 --cut-on-SCC-entry --simplify-input --powerset-for-weak --bscc-avoid --skip-levels --powerset-on-cut --cd > %O' \
         'ltl2tgba -D %f | ../seminator -s0 --cut-on-SCC-entry --simplify-input --powerset-for-weak --reuse-good-SCC --skip-levels --powerset-on-cut --cd > %O'
