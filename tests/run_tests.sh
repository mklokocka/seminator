# Run tests on random automata
ltlcross -F ../formulae/random_sd.ltl -F ../formulae/random_nd.ltl ltl2tgba 'ltl2tgba %f -D | ../seminator -s0 --cd > %O'  'ltl2tgba %f -D | ../seminator -s0 > %O'  'ltl2tgba %f -D | ../seminator --weak-powerset -s0 > %O'  'ltl2tgba %f -D | ../seminator -s0 --cd --jump-enter > %O'  'ltl2tgba %f -D | ../seminator -s0 --jump-always > %O'  'ltl2tgba %f -D | ../seminator --jump-enter --weak-powerset -s0 > %O'

