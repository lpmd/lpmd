#!/usr/bin/env python3

import sys, builtins, os, os.path

fakereadlink = lambda x: os.readlink(x) if os.path.islink(x) else x
exename = os.path.abspath(os.path.join(os.path.dirname(sys.argv[0]), fakereadlink(sys.argv[0])))
mydir = os.path.join(os.path.dirname(os.path.dirname(exename)), 'lib')
sys.path = [mydir]+sys.path
from lptoolkit import LPMD

def ProcessControlObject(s, from_where):
    NonModuleOptions = lambda z: dict(x for x in z.items() if x[0] != 'module')
    def Counter(name):
        k = int(parameters['counter-'+name])
        parameters['counter-'+name] += 1
        return k
    try:
      c = LPMD.ControlFile()
      c.ReadString(s)
      for name in c:
          for item in c[name]:
              if type(item) == LPMD.Statement:
                 if 'module' in item.positional:
                    tmp_pm = LPMD.PluginManager([os.path.join(mydir, 'lpmd')])
                    tmp_pm.Load(item.options['module'], alias='query_module')
                    pos_args = tmp_pm['query_module'].Parameters()
                    while len(item.positional['module']) > 0:
                          token, arg = item.positional['module'].pop(0), pos_args.pop(0)
                          item.options[arg] = token
                    del tmp_pm
                 if name == 'cell':
                    if 'a' in item.options and 'b' not in item.options:
                       item.options['b'], item.options['c'] =  item.options['a'], item.options['a']
                    parameters['cell'] = LPMD.Cell(a=float(item.options['a']), b=float(item.options['b']), c=float(item.options['c']))
                 elif name in ('input', 'output'):
                    pm.Load(item.options['module'], NonModuleOptions(item.options), alias=('%s%d' % (name, Counter(name))))
                 elif name == 'prepare':
                    pm.Load(item.options['module'], NonModuleOptions(item.options), alias=('prepare%d' % Counter('prepare')))
                 elif name == 'set': parameters[item.options['key']] = item.options['value']
                 elif name == 'cellmanager': parameters['cellmanager'] = item.options['module']
                 elif name == 'integrator': parameters['integrator'] = item.options['module']
                 elif name == 'potential':
                      # FIXME add the pairs information into the potential options
                      parameters['potential'].append(item.options['module'])
                 elif name == 'steps': parameters['steps'] = item.options['number']
                 elif name in ('monitor', 'apply', 'property'): parameters[name].append(item.options)
                 #else: print ('STATEMENT ', name, item.options, item.positional)
                 if item.extended != None: pass # print ('-> EXTENDED ', item.extended.options, item.extended.positional)
              else: pm.Load(item.name, options=item.options, alias=item.alias)
    except RuntimeError as e:
      print ('[Error] Reading %s: %s' % (from_where, str(e)))
      sys.exit(1)

def MustRun(stepper, step):
    if int(stepper['start']) > step: return False
    if stepper['end'] != '-1' and int(stepper['end']) < step: return False
    return (step % int(stepper['each']) == 0)

GetMonitorFormatter = lambda monitor: ' '.join('{%s}' % (z.replace('-', '')) for z in monitor['properties'].split(','))

def AutoFormatter(x):
    if isinstance(x, tuple) and len(x) == 3: # Vector
       return '%8.8f  %8.8f  %8.8f' % x
    if isinstance(x, LPMD.Matrix): # Matrix
       s = '# '
       for col in range(x.size[1]): s += ('%s  ' % x.GetLabel(col))
       s += '\n'
       for j in range(x.size[1]):
           for i in range(x.size[0]):
               s += ('%8.8f  ' % x.Get(i, j))
           s += '\n'
       return s.rstrip('\n')
    else: return str(x)

def AccumulateToOutputStream(value, stream):
    # FIXME This opens and closes the file stream every time, could be inefficient
    (sys.stdout if stream == '-' else open(stream, 'a')).write(AutoFormatter(value)+'\n')

def RunSimulation(parameters):
    timer = LPMD.Timer()
    try:
       print = lambda *args: (builtins.print(*args) if sim.topology.rank == 0 else None)
       sim = LPMD.Simulation(grid=grid, cell=parameters['cell'], pluginpath=[os.path.join(mydir, 'lpmd')])
       sim.atomset.boundarylength = float(parameters['boundary-length'])
       pm['input0'].Generate(sim, modify=True)
       if 'cellmanager' in parameters: sim.cellmanager = pm[parameters['cellmanager']]
       if 'integrator' in parameters: sim.solver = pm[parameters['integrator']]
       for p in parameters['potential']: sim.potentials.append(pm[p])
       print ('LPMD (in %s mode) running with processor grid: %s' % (app_name, sim.grid))
       def PostStep(sim, step):
           for mon in parameters['monitor']:
               if MustRun(mon, step): print (GetMonitorFormatter(mon).format(**sim.standard_properties))
           for k in range(parameters['counter-output']):
               if MustRun(pm['output%d' % k].options, step): pm['output%d' % k].WriteCell(sim)
           for sm in parameters['apply']:
               if MustRun(sm, step): pm[sm['module']].Apply(sim, modify=True)
           for prop in parameters['property']:
               if MustRun(prop, step):
                  value = pm[prop['module']].Evaluate(sim, modify=False)
                  AccumulateToOutputStream(value, pm[prop['module']].options['output'] if 'output' in pm[prop['module']].options else '-')
       sim.poststep = PostStep
       for k in range(parameters['counter-prepare']): pm['prepare%d' % k].Apply(sim, modify=True)
       for k in range(parameters['counter-output']): pm['output%d' % k].WriteHeader(sim)
       sim.Run(int(parameters['steps']))
    except RuntimeError as e: print ('[Error] %s' % str(e))
    except KeyboardInterrupt: pass
    finally:
       for k in range(parameters['counter-output']): pm['output%d' % k].CloseFile(sim)
       timer.Stop()
       print("\n#\n#\n#\n")
       print("Elapsed time: ", timer.elapsed, " seconds.")
       print("%4.2f%% of the time was lost in communication and high-level stuff.\n" % (100.0*(1.0-(sim.atomset.TaskTime()/timer.elapsed))))

#
#
#

# Recognize calling mode from sys.argv[0]
app_name = os.path.basename(sys.argv[0])
# Find a better way to work out the hint when the name of the utility is changed by a suffix (-testing)
use_hint = {'lpmd': 'apply', 'lpmd-testing': 'apply', 'lpmd-analyzer': 'property', 
            'lpmd-converter': 'apply', 'lpmd-visualizer': 'visualize'}[app_name]

nprocs = LPMD.InitializeCommunication(sys.argv)
q, pm = LPMD.QuickMode(use_hint), LPMD.PluginManager([os.path.join(mydir, 'lpmd')])

cell, parameters = None, {'potential': list(), 'monitor': list(), 
                          'apply': list(), 'property': list(), 'boundary-length': 8.5 }

for k in ('input', 'output', 'prepare'): parameters['counter-'+k] = 0
default_monitor = {'start': '0', 'end': '-1', 'each': '10', 'properties': 'step,total-energy'}

ProcessControlObject(q.GenerateControl(), 'quickmode flags')
for control in q.arguments: ProcessControlObject(open(control, 'r').read(), control)

grid = tuple(int(z) for z in parameters['grid'].split('x')) if 'grid' in parameters else LPMD.AutoGrid(nprocs)
if len(parameters['monitor']) == 0: parameters['monitor'] = [default_monitor]

if 'help_plugin' in parameters: pm.Load(parameters['help_plugin']).ShowDocumentation()
elif 'test_plugin' in parameters: pm.Load(parameters['test_plugin']).PerformTests()
else: RunSimulation(parameters)

LPMD.FinishCommunication()

