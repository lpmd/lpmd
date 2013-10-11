#!/usr/bin/env python3

import sys, builtins, LPMD

nprocs = LPMD.InitializeCommunication(sys.argv)
timer = LPMD.Timer()
grid=LPMD.AutoGrid(nprocs)

sim = LPMD.Simulation(grid=grid, cell=LPMD.Cell(a=57.036, b=57.036, c=57.036))

print = lambda *args: (builtins.print(*args) if sim.topology.rank == 0 else None)
print ('simplemd.py demo running with processor grid: ', sim.grid)

pm = LPMD.PluginManager()

writer = pm.Load('xyz', options={ 'file': 'output.xyz' })
gen = pm.Load('fcc', options={ 'nx': '10', 'ny': '10', 'nz': '10', 'a': '5.7036' })
#gen = pm.Load('randomconfig', options={ 'n': '4000' })
pot = pm.Load('lennardjones', options={'sigma': '3.41', 'epsilon': '0.0103048', 'cutoff': '8.5'})
temp = pm.Load('temperature', options={ 't': '300.0' })
integ = pm.Load('velocityverlet', options={ 'dt': '1.0' })
cellman = pm.Load('linkedcell', options={'cutoff': '8.5', 'nx': '15', 'ny': '15', 'nz': '15' })
tagger = pm.Load('settag', options={'key': LPMD.Tag('bala'), 'value': 'true'})
filter = pm.Load('index', options={'from': '2500', 'to': '2999'})

print ('Tag \"bala\" assigned to numeric tag %s' % LPMD.Tag('bala'))   # ya se uso antes, se recicla el valor ya asignado (0)
print ('Tag \"pared\" assigned to numeric tag %s' % LPMD.Tag('pared')) # nuevo tag, se asigna a 1

# Esto hara abortar el demo si se descomenta, ya que se pasa de las 64 tags permitidas
# for z in range(65): mynewtag = LPMD.Tag('tag%d' % z)

sim.atomset.boundarylength = 8.5
gen.Generate(sim, modify=True)
print ('Total Size: ', sim.atomset.totalsize)
assert sim.atomset.totalsize == 4000
temp.Apply(sim, modify=True)
sim.cellmanager = cellman       # Asigna el cellmanager
sim.solver = integ              # Asigna el integrator
sim.potentials.append(pot)      # Asigna los potenciales

def TestFiltersAndTags(sim):
    # Aplicar filtros es tan facil como encerrar el task a filtrar entre ApplyAtomMask y RemoveAtomMask
    sim.ApplyAtomMask(filter.Select)
    tagger.Apply(sim, modify=True)
    sim.RemoveAtomMask()

def PreStep(sim, step):
    if step == 0: print ('Initial potential energy: ', sim.potentialenergy)
    TestFiltersAndTags(sim)
    return (writer.WriteCell(sim) if step % 100 == 0 else None)

output_format = "Step {step}, U={potentialenergy:5.5f}, K={kineticenergy:5.5f}, E={totalenergy:5.5f}"
output_format += ", T={temperature:5.5f}, |P|={linearmomentum:5.5g}"

def PostStep(sim, step):
    return (print(output_format.format(**sim.standard_properties)) if step % 25 == 0 else None)

sim.prestep = PreStep    # Funcion callback llamada antes de cada paso
sim.poststep = PostStep  # Funcion callback llamada despues de cada paso

writer.WriteHeader(sim)
sim.Run(1000)            # Ejecuta 5000 pasos de simulacion
writer.CloseFile(sim)
timer.Stop()

print("\n#\n#\n#\n")
print("Elapsed time: ", timer.elapsed, " seconds.")
print("%4.2f%% of the time was lost in communication and high-level stuff.\n" % (100.0*(1.0-(sim.atomset.TaskTime()/timer.elapsed))))

LPMD.FinishCommunication()

