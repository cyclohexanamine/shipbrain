#include "neural.h"


float sigmoid(float f)
{
	return 1. / (1 + exp(-f));
}

float restrictangle(float a)
{
	while (a > PI) a -= 2*PI;
	while (a < -PI) a += 2*PI;
	return a;
}


Neuron::Neuron()
{
	val = 1.0;
	newval = 0;
	nconnect = 0;
	inputs = 0;
	weights = 0;
}
	
void Neuron::connect(Neuron* newinput, float newweight)
{
	Neuron** newnl = new Neuron*[nconnect+1];
	memcpy(newnl, inputs, nconnect*sizeof(Neuron*));
	newnl[nconnect] = newinput;
	delete inputs;
	inputs = newnl;
	
	float* newwt = new float[nconnect+1];
	memcpy(newwt, weights, nconnect*sizeof(float));
	newwt[nconnect] = newweight;
	delete weights;
	weights = newwt;
	
	nconnect++;
}

void Neuron::update()
{
	float sum = 0.;
	for (int i = 0; i < nconnect; i++)
		sum += inputs[i]->val * weights[i];
	newval = sigmoid(sum);		
}

	
	
Network::Network(int n_i, int n_o, int n_h)
{
	neurons = new Neuron[1+n_i+n_o+n_h];
	n_input = n_i;
	n_output = n_o;
	n_hidden = n_h;
}
	
void Network::update(float* inputs)
{
	neurons[0].val = 1.0;
	for (int i = 0; i < n_input; i++)
		neurons[1+i].val = inputs[i];
	
	for (int i = 0; i < n_output+n_hidden; i++)
		neurons[1+n_input+i].update();
	for (int i = 0; i < n_output+n_hidden; i++)
		neurons[1+n_input+i].val = neurons[1+n_input+i].newval;
}

float* Network::readin()
{
	float* out = new float[n_input];
	for (int i = 0; i < n_input; i++)
		out[i] = neurons[1+i].val;
	return out;
}

float* Network::readoutnodes()
{
	float* out = new float[n_output];
	for (int i = 0; i < n_output; i++)
		out[i] = neurons[1+n_input+i].val;
	return out;
}

bool* Network::readout()
{
	bool* out = new bool[n_output];
	for (int i = 0; i < n_output; i++)
		out[i] = neurons[1+n_input+i].val > 0.5;
	return out;
}





Network* shipmind1() // just turns to face the player and fires continuously
{
	Network* ret = new Network(6, 7, 0);
	
	ret->neurons[9].connect(&ret->neurons[4], 1.0f); // + rotation for +angle
	ret->neurons[9].connect(&ret->neurons[2], -2.0f); // + rotation againt +av
	
	ret->neurons[10].connect(&ret->neurons[4], -1.0f); // same as ^ for - rotation
	ret->neurons[10].connect(&ret->neurons[2], 2.0f);
	
	ret->neurons[13].connect(&ret->neurons[0], 1.0f); // bias to fire
	
	return ret;
}

Network* shipmind2() // turns to face the player and fires when it's aiming at the player
{
	Network* ret = new Network(6, 7, 2);
	
	ret->neurons[9].connect(&ret->neurons[4], 1.0f); // + rotation for +angle
	ret->neurons[9].connect(&ret->neurons[2], -2.0f); // + rotation againt +av
	
	ret->neurons[10].connect(&ret->neurons[4], -1.0f); // same as ^ for - rotation
	ret->neurons[10].connect(&ret->neurons[2], 2.0f);
	
	
	ret->neurons[14].connect(&ret->neurons[4], -10);
	ret->neurons[14].connect(&ret->neurons[0], 2); // angle < 0.1
	
	ret->neurons[15].connect(&ret->neurons[4], 10);
	ret->neurons[15].connect(&ret->neurons[0], 2); // angle > -0.1
	
	ret->neurons[13].connect(&ret->neurons[14], 1.0f); // fire on AND
	ret->neurons[13].connect(&ret->neurons[15], 1.0f);
	ret->neurons[13].connect(&ret->neurons[0], -1.2f);
	
	
	return ret;
}

