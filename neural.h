#ifndef NEURAL_H
#define NEURAL_H

#include <string.h>
#include <math.h>
#include <time.h>
#include <random>
#include <algorithm>
#include <fstream>
#include <climits>

/**
positions are done in radial coordinates relative to the nose of the ship

 INPUT:
enemy ship
1	r [0, inf]
2	theta [-pi, pi]
3	r' [-inf, inf]
4	theta' [-inf, inf]
5	orientation [-pi, pi]
6 	angular velocity [-inf, inf]
	
	
 OUTPUT:
7	W - forward
8	S - backward
9	A - rotate +
10	D - rotate -
11	Q - translate -
12	E - translate +
13	space - fire

**/

#define PI 3.14159265

#define NINPUTS 6
#define NOUTPUTS 7

#define MutateConnectionsChance 0.25
#define PerturbChance 0.90
#define CrossoverChance 0.75
#define LinkMutationChance 2.0
#define NodeMutationChance 0.50
#define BiasMutationChance 0.40
#define StepSize 0.1
#define DisableMutationChance 0.4
#define EnableMutationChance 0.2

#define DeltaDisjoint 2.0
#define DeltaWeights 0.4
#define DeltaThreshold 1.0

#define StaleLimit 15

#define NPOP 300
#define MAXSPECIES 25
#define MINSPECIES 5
#define KEEPOLD 5


float sigmoid(float f);
float restrictangle(float a);

extern bool init_rand;
extern std::mt19937 device;
extern std::uniform_real_distribution<double> distribution;
float randfloat();
int randint(int min, int max);

template<typename T> void listappend(T*& list, int& listsize, T item);
template<typename T> void listshorten(T*& list, int newsize);
template<typename T> void listremove(T*& list, int& listsize, int listpos);

extern int global_innov;
int newinnov();


class Neuron
{
	public:
	
	Neuron();
	
	float val;
	float newval;
	int nconnect;
	Neuron** inputs;
	float* weights;
	
	void connect(Neuron* newinput, float newweight);
	void update();
};


class Network
{
	public:
	
	Network(int n_i, int n_o, int n_h);
	
	Neuron* neurons;
	int n_input;
	int n_output;
	int n_hidden;
	
	void update(float* inputs);
	
	float* readin();
	float* readoutnodes();
	bool* readout();
};

class Gene
{
	public:
	
	int in;
	int out;
	float weight;
	int innov;
	bool enabled;
};

class Genome
{
	public:
	
	Genome(int n_i, int n_o, int n_h=0, int ng=0);
	Genome* copy();
	void addgene(int n_in, int n_out, float weight, int innovation, bool enabled=true);
	
	int n_input;
	int n_output;
	int n_hidden;
	
	int fitness;
	
	int ngenes;
	Gene* genes;
	
	Network* makenetwork();
};

class Species
{
	public: 
	
	Species();
	
	int topfitness;
	int avgfitness;
	int staleness;
	int ngenomes;
	Genome** genomes;
};

class Population
{
	public:
	
	Population(Genome** genomes, int ngenomes);
	
	void addtospecies(Genome** genomes, int ngenomes);
	
	int nspecies;
	Species** species;
	
	int globalfitness;
	
	void calcfitness(int (*evalfitness)(Genome*));
	void nextgen();
};



int genome_fitness_comp(const void* as1, const void* as2);
int species_fitness_comp(const void* as1, const void* as2);

float disjoint(Genome* g1, Genome* g2);
float weightdiff(Genome* g1, Genome* g2);
bool samespecies(Genome* g1, Genome* g2);

int randomneuron(Genome* g, bool in=true, bool out=true, bool hidden=true);

void mutateconnections(Genome* g);
void newlink(Genome* g);
void newbias(Genome* g);
void newnode(Genome* g);
void disablemutate(Genome* g);
void activationmutate(Genome* g, bool enable);

void doprob(Genome* g, float p, void(*act)(Genome*));

Genome* mutate(Genome* oldg);
Genome* cross(Genome* g1, Genome* g2);

void writegenome(Genome* g, const char* fname);
Genome* readgenome(const char* fname);


#endif