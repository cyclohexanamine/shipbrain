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




bool init_rand = false;
std::mt19937 device;
std::uniform_real_distribution<double> distribution(0, 1);
float randfloat()
{
	if (!init_rand)
	{
		device.seed(time(0));
		init_rand = true;
	}
	
	return (float)distribution(device);
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

Genome::Genome(int n_i, int n_o)
{
	n_input = n_i;
	n_output = n_o;
	n_hidden = 0;
	ngenes = 0;
	genes = 0;
}

Genome* Genome::copy()
{
	Genome* ret = new Genome(n_input, n_output);
	ret->n_hidden = n_hidden;
	ret->ngenes = ngenes;
	ret->genes = new Gene[ngenes];
	memcpy(ret->genes, genes, ngenes*sizeof(Gene));
	return ret;
}


void Genome::addgene(int n_in, int n_out, float weight, bool enabled)
{
	Gene* newgl = new Gene[ngenes+1];
	memcpy(newgl, genes, ngenes*sizeof(Gene));
	newgl[ngenes] = {n_in, n_out, weight, enabled};
	delete genes;
	genes = newgl;
	
	int topn = std::max(n_in, n_out) - n_input + n_output + n_hidden;
	if (topn > 0)
		n_hidden += topn;
	
	ngenes++;
}

Network* Genome::makenetwork()
{
	Network* ret = new Network(n_input, n_output, n_hidden);
	for (int i = 0; i < ngenes; i++)
	{
		if (!genes[i].enabled) continue;
		ret->neurons[genes[i].out].connect(&ret->neurons[genes[i].in], genes[i].weight);
	}
	
	return ret;
}


Genome* mutate(Genome* oldg)
{
	Genome* ng = oldg->copy();
	if (true) // (randfloat() < MutateConnectionsChance
	for (int i = 0; i < ng->ngenes; i++)
	{
		if (randfloat() < PerturbChance)
			ng->genes[i].weight += (2*randfloat()-1) * StepSize;
		else
			ng->genes[i].weight = 4*randfloat() - 2;
	}
	
	return ng;
}





Genome* shipmind()
{
	Genome* g = new Genome(6, 7);
	
	g->addgene(2, 9, 1.);
	g->addgene(4, 9, 2.);
	g->addgene(2, 10, -1.);
	g->addgene(4, 10, -2.);
	
	g->addgene(2, 14, -10.);
	g->addgene(0, 14, 2.);	
	g->addgene(2, 15, 10.);
	g->addgene(0, 15, 2.);
	
	g->addgene(0, 13, 1.0);
	
	return g;
}

Genome* shipmind2()
{
	Genome* g = new Genome(6, 7);
	
	g->addgene(2, 9, 1.);
	g->addgene(4, 9, 2.);
	g->addgene(2, 10, -1.);
	g->addgene(4, 10, -2.);
	
	g->addgene(2, 14, -10.);
	g->addgene(0, 14, 2.);	
	g->addgene(2, 15, 10.);
	g->addgene(0, 15, 2.);
	
	g->addgene(14, 13, 1.);
	g->addgene(15, 13, 1.);
	g->addgene(0, 13, -1.2);
	
	return g;
}
