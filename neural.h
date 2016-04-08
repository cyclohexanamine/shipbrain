#ifndef NEURAL_H
#define NEURAL_H

#include <string.h>
#include <math.h>

/**
displacement and velocity vectors are relative to the reference frame looking along the nose of the ship, i.e., input 1 is always 0

 INPUT:
 ship 
1	orientation [-pi, pi]
2 	angular velocity [-inf, inf]
 enemy ship
3	distance [0, inf]
4	angle to [-pi, pi]
5	angle [-pi, pi]
6	angular velocity [-inf, inf]
	
	
 OUTPUT:
7	W - forward
8	S - backward
9	A - rotate +
10	D - rotate -
11	Q - translate -
12	E - translate +
13	space - fire

**/


#define NINPUTS 6
#define NOUTPUTS 7
#define PI 3.14159265

float sigmoid(float f);
float restrictangle(float a);

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