#ifndef NEURAL_H
#define NEURAL_H

#include <string.h>
#include <math.h>
#include <random>

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

#define MutateConnectionsChance = 0.25
#define PerturbChance = 0.90
#define CrossoverChance = 0.75
#define LinkMutationChance = 2.0
#define NodeMutationChance = 0.50
#define BiasMutationChance = 0.40
#define StepSize = 0.1
#define DisableMutationChance = 0.4
#define EnableMutationChance = 0.2

/*
genome.mutationRates["connections"] = MutateConnectionsChance
genome.mutationRates["link"] = LinkMutationChance
genome.mutationRates["bias"] = BiasMutationChance
genome.mutationRates["node"] = NodeMutationChance
genome.mutationRates["enable"] = EnableMutationChance
genome.mutationRates["disable"] = DisableMutationChance
genome.mutationRates["step"] = StepSize
*/





float sigmoid(float f);
float restrictangle(float a);

float randfloat();
extern std::default_random_engine generator;
extern std::uniform_real_distribution<float> distribution;

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
	int n_hidden;
	int n_output;
	
	void update(float* inputs);
	
	float* readin();
	float* readoutnodes();
	bool* readout();
};



Network* shipmind1();
Network* shipmind2();



#endif