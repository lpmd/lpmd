
#include <cstdlib>
#include <unistd.h>
#include <string>
#include <iostream>
#include <fstream>
#include <cassert>
#include <cmath>
#include <ctime>
#include <mpi.h>

#include <lpmd/array.h>
#include <lpmd/parameter.h>
#include <lpmd/atom.h>
#include <lpmd/storedconfiguration.h>
#include <lpmd/pluginmanager.h>
#include <lpmd/potential.h>
#include <lpmd/cellmanager.h>
#include <lpmd/session.h>
#include <lpmd/util.h>

using namespace std;
using namespace lpmd;

/////////////////////////////////////////////////////////

#define natoms 2048    // Maximum number of atoms in each configuration
#define dim 5          // Number of parameters of the potential function

#define n_part 64       // Number of "particles" used by PSO. Not the same as number of atoms!
#define error_tol 5.0  // Tolerance for the error of fitting.

std::string par_names[dim] = { "a", "n", "c", "m", "e" };
double xmin[dim] = { 0.0, 0.0, 0.0, 0.0, 0.0 };      // Minimum values of the parameters for the potential
double xmax[dim] = { 4.0, 12.0, 30.0, 12.0, 0.05 };   // Maximum values of the parameters for the potential

/////////////////////////////////////////////////////////

double xtglobal[dim];
double vtglobal[dim];

double x_0[n_part][dim];
double v_0[n_part][dim];
double x_best[n_part][dim];
double x_global[dim];
double bestval[n_part];

#define BOHR2ANGSTROM 0.52917721092
#define HARTREE2EV 27.211385

double w = 0.7;       // w, c_1 and c_2 are initial parameters for the PSO algorithm
double c_1 = 1.4;
double c_2 = 1.4;

int IsLocalProcessor(int j)
{
 int rank, s;
 MPI_Comm_rank(MPI_COMM_WORLD, &rank);
 MPI_Comm_size(MPI_COMM_WORLD, &s);
 return ((j % s) == rank);
}

double * v_t(double x_0[], double v_0[], double x_best[], double x_global[], double R1, double R2)
{
 for(int i=0;i < dim;i=i+1)
     vtglobal[i] = w*v_0[i] + c_1*R1*(x_best[i]-x_0[i]) + c_2*R2*(x_global[i]-x_0[i]); 
 return vtglobal;
}

double * x_t(double x_0[], double v_t[])
{
 for (int i=0;i < dim;i=i+1)
     xtglobal[i] = x_0[i] + v_t[i]; 
 return xtglobal;
}

double Random() { return double(rand())/double(RAND_MAX); }

double * ReadDataRuNNer(lpmd::Array<lpmd::StoredConfiguration> & allconfigs)
{
 ifstream infile;
 std::string line;
 infile.open("input.data");
 int n = 0, atomn = 0, celln = 0;
 lpmd::StoredConfiguration this_config;
 double * energies = new double[10000];
 while (1)
 {
  getline(infile, line);
  if (infile.eof()) break;
  Array<string> words = StringSplit(line);
  if (words.Size() < 1) continue;
  if (words[0] == "begin") 
  { 
   atomn = 0; 
   celln = 0; 
   this_config.Cell() = Cell();
   this_config.Atoms().Clear();
  }
  else if (words[0] == "end") 
  { 
   allconfigs.Append(this_config);
   n++;
  }
  else if (line[0] == 'c') { }     // Read comment line
  else if (words[0] == "lattice")
  {
   Vector v = Vector(BOHR2ANGSTROM*double(Parameter(words[1])), BOHR2ANGSTROM*double(Parameter(words[2])), BOHR2ANGSTROM*double(Parameter(words[3])));
   this_config.Cell()[celln] = v;
   celln++;
  }
  else if (words[0] == "atom") 
  { 
   double x, y, z;
   x = BOHR2ANGSTROM*double(Parameter(words[1]));
   y = BOHR2ANGSTROM*double(Parameter(words[2]));
   z = BOHR2ANGSTROM*double(Parameter(words[3]));
   const Vector pos(x, y, z);
   this_config.Atoms().Append(Atom(std::string(words[4]), pos));
   atomn++;
  }
  else if (words[0] == "energy") { energies[n] = HARTREE2EV*double(Parameter(words[1])); }
  else if (words[0] == "charge") { } // Charge is ignored
 }
 infile.close();
 return energies;
}

double EnergyError(lpmd::PluginManager * pm, lpmd::CellManager & cm, lpmd::Array<lpmd::StoredConfiguration> & allconfigs, double * params, double * energies)
{
 double S = 0.0;
 std::string param_str = "";
 for (int k=0;k<dim;k++) param_str += (par_names[k]+" "+ToString(params[k])+" ");
 pm->UpdatePlugin("pot", "debug none cutoff 6.0 "+param_str);
 Potential & pot = dynamic_cast<Potential &>((*pm)["pot"]);
 for (int i=0;i<allconfigs.Size();++i)
 {
  cm.UpdateCell(allconfigs[i]);
  pot.UpdateForces(allconfigs[i]);
  S += pow(pot.energy(allconfigs[i])-energies[i],2.0);
 } 
 S = 1000.0*sqrt(S/double(allconfigs.Size()))/double(allconfigs[0].Atoms().Size()); // error in meV/atom
 //cout << "- New evaluation of " << param_str << " yields error of " << S << " meV/atom" << endl;
 return S;
}

void AdvanceParticles(lpmd::PluginManager * pm, lpmd::CellManager & cm, lpmd::Array<lpmd::StoredConfiguration> & allconfigs, double * energies, double & bestvalue) 
{
 for (int p=0;p<n_part;p++)
 {
  if (!IsLocalProcessor(p)) continue;
  double v_new[dim], x_new[dim];
  double R1 = Random();
  double R2 = Random();
  for (int a=0; a<dim; a++)
  {
   v_new[a] = v_t(x_0[p], v_0[p], x_best[p], x_global, R1, R2)[a];
   x_new[a] = x_t(x_0[p], v_new)[a];
   while (x_new[a] < xmin[a]) x_new[a] = x_new[a] + (xmax[a]-xmin[a]);
   while (x_new[a] > xmax[a]) x_new[a] = x_new[a] - (xmax[a]-xmin[a]);
  }
  for (int a=0;a<dim;++a) 
  {
   x_0[p][a] = x_new[a];
   v_0[p][a] = v_new[a];
  }
  double vfnew = EnergyError(pm, cm, allconfigs, x_new, energies);
  if (vfnew < bestval[p])
  {
   for (int a=0;a<dim;a++) x_best[p][a] = x_new[a];
   bestval[p] = vfnew;
  }
  if (vfnew < bestvalue)
  {
   for(int a=0; a<dim; a++) { x_global[a] = x_new[a]; }
   bestvalue = vfnew;
  }
 }
}

int main(int argc, char *argv[])
{

 MPI_Init(&argc, &argv);

 int nprocs, rank;
 MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
 MPI_Comm_rank(MPI_COMM_WORLD, &rank);

 srand(rank*time(0));

 lpmd::PluginManager * pm = new lpmd::PluginManager();
 pm->LoadPlugin("linkedcell", "cm", "nx 8 ny 8 nz 8 cutoff 6.0");
 //pm->LoadPlugin("minimumimage", "cm", "cutoff 6.0");

 CellManager & cm = dynamic_cast<CellManager &>((*pm)["cm"]);
 
 lpmd::GlobalSession.SetDebugFile("debug.out");
 pm->LoadPlugin("suttonchen", "pot", "debug none cutoff 6.0");

 Potential & pot = dynamic_cast<Potential &>((*pm)["pot"]);

 lpmd::Array<lpmd::StoredConfiguration> allconfigs;
 double * allenergies = ReadDataRuNNer(allconfigs);

 if (rank == 0)
   for (int i=0;i<allconfigs.Size();++i) 
   { 
    allconfigs[i].ShowInfo(cout);
   }

 for (int i=0;i<allconfigs.Size();++i) allconfigs[i].SetCellManager(cm);
 
 pot.Initialize(allconfigs[0]);

 for (int i=0; i<dim; i++) { x_global[i] = xmin[i] + (xmax[i]-xmin[i])*0.5; }

 //x_global[0] = 3.44037;  
 //x_global[1] = 8.18152;
 //x_global[2] = 20.3203;
 //x_global[3] = 7.87174;
 //x_global[4] = 0.0411011;

 for (int j=0;j<n_part;j++)
 {
  if (!IsLocalProcessor(j)) continue;
  for (int i=0; i<dim; i++)
  {
   x_best[j][i] = x_global[i];
   x_0[j][i] = xmin[i] + (xmax[i]-xmin[i])*Random();
   v_0[j][i] = 2.0*(2.0*Random()-1.0);
  }
 }

 double bestvalue = EnergyError(pm, cm, allconfigs, x_global, allenergies);
 for (int p=0;p<n_part;++p)
 {
  if (IsLocalProcessor(p)) bestval[p] = bestvalue;
 }

 double * allbestvalues = NULL;
 double * allglobals = NULL;
 if (rank == 0)
 {
  allbestvalues = (double *)malloc(nprocs*sizeof(double));
  for (int i=0;i<nprocs;++i) allbestvalues[i] = 1.0E+12;
  allglobals = (double *)malloc(nprocs*dim*sizeof(double));
 }

 int i=1;
 while (bestvalue > error_tol)
 {
  AdvanceParticles(pm, cm, allconfigs, allenergies, bestvalue);
  // 
  // Collect bestvalue and x_global from all MPI nodes
  // 
  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Gather(&bestvalue, 1, MPI_DOUBLE, allbestvalues, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Gather(x_global, dim, MPI_DOUBLE, allglobals, dim, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  MPI_Barrier(MPI_COMM_WORLD);
  if (rank == 0)
  {
   for (int p=0;p<nprocs;p++) 
       if (allbestvalues[p] < bestvalue)
       {
        bestvalue = allbestvalues[p];
        for (int q=0;q<dim;q++) x_global[q] = allglobals[p*dim+q]; 
       }
  }
  //
  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Bcast(&bestvalue, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Bcast(x_global, dim, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  MPI_Barrier(MPI_COMM_WORLD);
  //

  // Collect statistics from all particles in all MPI nodes
  double local_Errsum = 0.0, global_Errsum = 0.0;
  double local_Err2sum = 0.0, global_Err2sum = 0.0;
  for (int p=0;p<n_part;p++)
      if (IsLocalProcessor(p)) 
      { 
       local_Errsum += bestval[p];
       local_Err2sum += pow(bestval[p],2.0);
      }
  MPI_Reduce(&local_Errsum, &global_Errsum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
  MPI_Reduce(&local_Err2sum, &global_Err2sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
  //
  if (rank == 0)
  {
   cout << "[" << i << "] ";
   for (int d=0; d<dim; d++) cout << x_global[d] << " ";
   cout << "(" << bestvalue << " meV/atom) [Swarm err=" << global_Errsum/n_part << " +/- " << sqrt(global_Err2sum/n_part) << "]" << endl;
  }
  i++;
 }

 if (rank == 0)
 {
  free(allbestvalues);
  free(allglobals);
  for (int d=0;d<dim;d++) { cout << x_global[d] << " " ; }
  cout << endl;
  cout << bestvalue << "\n Number of steps:" << i << endl;
 }

 delete pm;
 delete [] allenergies;

 MPI_Finalize();

 return 0;
}

