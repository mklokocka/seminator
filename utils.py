import spot

seminator_path = './seminator'
def seminator(filename, opt = ''):
    a = spot.automaton(f'{seminator_path} {opt} -s0 {filename} |')
    display(a)
    return a

def seminator_form(formula, opt = ''):
    a = spot.automaton(f'ltl2tgba \'{formula}\' | {seminator_path} {opt} -s0 |')
    display(a)
    return a

def highlight(f):
    a = spot.translate(f)
    spot.highlight_nondet_states(a,1)
    display(a.show('.s'))
