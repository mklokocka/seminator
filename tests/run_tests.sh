# Run tests on random automata
ltlcross -F ../formulae/random_sd.ltl -F ../formulae/random_nd.ltl ltl2tgba 'ltl2tgba %f -D | ../seminator -s0 --cd > %O'  'ltl2tgba %f -D | ../seminator -s0 > %O'

