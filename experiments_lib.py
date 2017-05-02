# coding: utf-8

import spot
import pandas as pd
import subprocess
from functools import lru_cache


def pretty_print(form):
    '''Runs Spot to format formulas nicer.
    '''
    return spot.formula(form).to_str()


def is_interesting(form):
    '''Runs LTL3HOA in order to check a given formula for a mergeable Until.

    Returns
    =======
    * `True` if `form` is mergeable
    * `False` otherwise
    '''
    res = subprocess.check_output(["ltl3hoa", "-m1", "-f", form])
    return bool(int(res))

# def is_interesting(f):
#    #print(f)
#    ltl3hoa = subprocess.Popen(['ltl3hoa', '-m', '-f', f],
#                           #stdin=subprocess.PIPE,
#                           stdout=subprocess.PIPE,
#                           stderr=subprocess.PIPE)
#    stdout, stderr = ltl3hoa.communicate()
#    if stderr:
#        print("Calling 'ltl3hoa' for the detection of mergeable Until produced the message:\n"
#              + stderr.decode('utf-8'), file=sys.stderr)
#    ret = ltl3hoa.wait()
#    if ret:
#        raise subprocess.CalledProcessError(ret, 'ltl3hoa')
#    return bool(int(stdout.decode('utf-8')))


def get_states_number(command,formula):
    '''Runs the formatable command on given formula and returns
    the number of states of the resulting automaton.'''
    output = get_ipython().getoutput('timeout 2s {command.format(formula)} | grep States:')
    try:
        ret = int(output[0].split()[1])
    except ValueError:
        ret = -1
    except IndexError:
        ret = -2
    return ret

    
def compute_results(formulas,toolnames,tools):
    '''Runs each tool from `toolnames` on each formula from `formulas`
    and stores the results in a pandas DataFrame, which is returned.'''
    data = [
        [i,str(spot.formula(formula)),is_interesting(str(spot.formula(formula))) ] +\
                 [get_states_number(tools[name],formula) for name in toolnames]
        for i,formula in enumerate(formulas)
    ]
    return pd.DataFrame.from_records(data,
            columns=['form_id','formula','interesting'] + toolnames,
            index='form_id')


# Add a small LRU cache so that when we display automata into a
# interactive widget, we avoid some repeated calls to dot for
# identical inputs.
@lru_cache(maxsize=64)
def dot_to_svg(str):
    """
    Send some text to dot for conversion to SVG.
    """
    dot = subprocess.Popen(['dot', '-Tsvg'],
                           stdin=subprocess.PIPE,
                           stdout=subprocess.PIPE,
                           stderr=subprocess.PIPE)
    stdout, stderr = dot.communicate(str.encode('utf-8'))
    if stderr:
        print("Calling 'dot' for the conversion to SVG produced the message:\n"
              + stderr.decode('utf-8'), file=sys.stderr)
    ret = dot.wait()
    if ret:
        raise subprocess.CalledProcessError(ret, 'dot')
    return stdout.decode('utf-8')

def hoa_to_dot(hoa):
    """
    Converts an HOA automaton into its DOT representation.
    Works only for nondeterministic automata.
    """
    autfilt = subprocess.Popen(['autfilt', '--dot'],
                           stdin=subprocess.PIPE,
                           stdout=subprocess.PIPE,
                           stderr=subprocess.PIPE)
    stdout, stderr = autfilt.communicate(hoa.encode('utf-8'))
    if stderr:
        print("Calling 'autfilt' for the conversion to DOT produced the message:\n"
              + stderr.decode('utf-8'), file=sys.stderr)
    ret = autfilt.wait()
    if ret:
        raise subprocess.CalledProcessError(ret, 'autfilt')
    return stdout.decode('utf-8')

def hoa_to_spot(hoa):
    print(hoa,file=open('tmp_aut.hoa','w'))
    a = spot.automaton('tmp_aut.hoa')
    subprocess.call(['rm','tmp_aut.hoa'])
    return a

def dot_for_vwaa(command,formula):
    """
    Adds `-o dot` to the command.
    """
    #TODO Only works if formula is the last argument. FIX IT!
    cmd = command.split(' ')[:-1] + [formula,'-o','dot']

    hoa = subprocess.Popen(cmd,
                           stdin=subprocess.PIPE,
                           stdout=subprocess.PIPE,
                           stderr=subprocess.PIPE)
    stdout, stderr = hoa.communicate()
    if stderr:
        print("Calling the translator produced the message:\n"
              + stderr.decode('utf-8'), file=sys.stderr)
    ret = hoa.wait()
    if ret:
        raise subprocess.CalledProcessError(ret, 'translator')
    return stdout.decode('utf-8')

def get_svg(command,formula):
    return dot_to_svg(dot_for_vwaa(command,formula))