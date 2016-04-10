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

int randint(int min, int max)
{
	if (!init_rand)
	{
		device.seed(time(0));
		init_rand = true;
	}	
	
	if (min>max) return 0;
	std::uniform_int_distribution<int> uni(min, max);
	return uni(device);
}




int global_innov = 100;
int newinnov()
{
	return global_innov++;
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
	listappend<Neuron*>(inputs, nconnect, newinput);
	nconnect--;
	listappend<float>(weights, nconnect, newweight);
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

Genome::Genome(int n_i, int n_o, int n_h, int ng)
{
	n_input = n_i;
	n_output = n_o;
	n_hidden = n_h;
	ngenes = ng;
	if (ng==0) genes = 0;
	else genes = new Gene[ng];
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


void Genome::addgene(int n_in, int n_out, float weight, int innovation, bool enabled)
{
	Gene* newgl = new Gene[ngenes+1];
	memcpy(newgl, genes, ngenes*sizeof(Gene));
	newgl[ngenes] = {n_in, n_out, weight, innovation, enabled};
	delete genes;
	genes = newgl;
	
	int topn = std::max(n_in, n_out) - (n_input + n_output + n_hidden);
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


Species::Species()
{
	topfitness = 0;
	avgfitness = 0;
	staleness = 0;
	ngenomes = 0;
	genomes = 0;
}


Population::Population(Genome** genomes, int ngenomes)
{
	nspecies = 0;
	species = 0;
	globalfitness = 0;
	
	Genome** newgs = new Genome*[NPOP];
	for (int i = 0; i < NPOP; i++)
	{
		newgs[i] = mutate(genomes[i%ngenomes]);
	}
	
	addtospecies(newgs, NPOP);
}

void Population::addtospecies(Genome** genomes, int ngenomes)
{
	for (int i = 0; i < ngenomes; i++)
	{
		bool found = false;
		for (int j = 0; j < nspecies; j++)
		{
			if (samespecies(genomes[i], species[j]->genomes[0]))
			{
				listappend<Genome*>(species[j]->genomes, species[j]->ngenomes, genomes[i]);
				found = true;
				break;
			}
		}
		
		if (!found)
		{
			Species* ns = new Species;
			listappend<Genome*>(ns->genomes, ns->ngenomes, genomes[i]);
			listappend<Species*>(species, nspecies, ns);
		}
	}
}

void Population::calcfitness(int (*evalfitness)(Genome*))
{
	globalfitness = 0;
	for (int i = 0; i < nspecies; i++)
	{
		int sum = 0;
		for (int j = 0; j < species[i]->ngenomes; j++)
		{
			species[i]->genomes[j]->fitness = evalfitness(species[i]->genomes[j]);
			sum += species[i]->genomes[j]->fitness;
		}
		qsort(species[i]->genomes, species[i]->ngenomes, sizeof(Genome*), genome_fitness_comp);
		if (species[i]->topfitness >= species[i]->genomes[0]->fitness) species[i]->staleness += 1;
		species[i]->topfitness = species[i]->genomes[0]->fitness;
		species[i]->avgfitness = sum/species[i]->ngenomes;
		globalfitness += species[i]->avgfitness;
	}
	globalfitness /= nspecies;
	
	qsort(species, nspecies, sizeof(Species*), species_fitness_comp);
}


void Population::nextgen()
{
	int nchildren = 0;
	Genome** children = 0;
	
	if (nspecies > MAXSPECIES)
	{
		nspecies = MAXSPECIES;
		listshorten<Species*>(species, nspecies);
	}
	
	for (int i = 0; i < nspecies; i++)
	{
		species[i]->ngenomes /= 2;
		listshorten<Genome*>(species[i]->genomes, species[i]->ngenomes);
		
		int nbreed = globalfitness ? species[i]->avgfitness / globalfitness * NPOP - 1 : 1;
		if (species[i]->ngenomes == 0 || (nspecies > MINSPECIES && ((nbreed <= 0 && nspecies) || (species[i]->staleness > StaleLimit && i != 0))))
		{
			delete species[i];
			listremove<Species*>(species, nspecies, i);
			continue;
		}
		
		for (int j = 0; j < nbreed; j++)
		{
			Genome* child = 0;
			if (randfloat() < CrossoverChance)
			{
				Genome* g1 = species[i]->genomes[randint(0, species[i]->ngenomes-1)];
				Genome* g2 = species[i]->genomes[randint(0, species[i]->ngenomes-1)];
				child = mutate(cross(g1, g2));
			}
			else
			{
				Genome* g1 = species[i]->genomes[randint(0, species[i]->ngenomes-1)];
				child = mutate(g1);
			}
			
			listappend<Genome*>(children, nchildren, child);
		}
		
		species[i]->ngenomes = std::min(KEEPOLD, species[i]->ngenomes);
		listshorten<Genome*>(species[i]->genomes, species[i]->ngenomes);
	}

	if (nchildren > NPOP - KEEPOLD * nspecies)
	{
		nchildren = NPOP - KEEPOLD * nspecies;
		listshorten<Genome*>(children, nchildren);
	}
	
	addtospecies(children, nchildren);
}




int genome_fitness_comp(const void* as1, const void* as2)
{
	Genome* g1 = *(Genome**)as1;
	Genome* g2 = *(Genome**)as2;
	
	if (g1->fitness < g2->fitness) return 1;
	if (g1->fitness == g2->fitness) return 0;
	if (g1->fitness > g2->fitness) return -1;
	return 0;
}
int species_fitness_comp(const void* as1, const void* as2)
{
	Species* s1 = *(Species**)as1;
	Species* s2 = *(Species**)as2;
	
	if (s1->avgfitness < s2->avgfitness) return 1;
	if (s1->avgfitness == s2->avgfitness)
	{
		if (s1->topfitness < s2->topfitness) return 1;
		if (s1->topfitness == s2->topfitness) return 0;
		if (s1->topfitness > s2->topfitness) return -1;
	}
	if (s1->avgfitness > s2->avgfitness) return -1;
	
	return 0;
}

float disjoint(Genome* g1, Genome* g2)
{
	int ndisj = 0;
	for (int i = 0; i < g1->ngenes; i++)
	{
		bool disj = true;
		for (int j = 0; j < g2->ngenes; j++)
		{
			if (g1->genes[i].innov == g2->genes[j].innov)
			{
				disj = false;
				break;
			}
		}
		if (disj) ndisj++;
	}
	
	return (float)ndisj / (float)std::max(g1->ngenes, g2->ngenes);
}

float weightdiff(Genome* g1, Genome* g2)
{
	float sum = 0;
	int incident = 0;
	for (int i = 0; i < g1->ngenes; i++)
	{
		for (int j = 0; j < g2->ngenes; j++)
		{
			if (g1->genes[i].innov == g2->genes[j].innov)
			{
				sum += std::abs(g1->genes[i].weight - g2->genes[i].weight);
				incident++;
				break;
			}
		}
	}
	
	if (incident==0) return 0;
	else return sum/(float)incident;
	
}

bool samespecies(Genome* g1, Genome* g2)
{
	return disjoint(g1, g2) * DeltaDisjoint + weightdiff(g1, g2) * DeltaWeights < DeltaThreshold;
}


int randomneuron(Genome* g, bool in, bool out, bool hidden)
{	
	int nmin = 1;
	int nmax = 0;
	if (in) nmax += g->n_input;
	if (out) nmax += g->n_output;
	if (hidden) nmax += g->n_hidden;
	
	if (nmin>nmax) return 0;
	int ind = randint(nmin, nmax);
	
	if (in)
	{
		if (ind <= g->n_input || out) return ind;
		else return ind + g->n_output;
	}
	else if (out)
	{
		return ind + g->n_input;
	}
	else
	{
		return ind + g->n_input + g->n_output;
	}
}

void mutateconnections(Genome* g)
{
	for (int i = 0; i < g->ngenes; i++)
	{
		if (randfloat() < PerturbChance)
			g->genes[i].weight += (2*randfloat()-1) * StepSize;
		else
			g->genes[i].weight = 4*randfloat() - 2;
	}
}

void newlink(Genome* g)
{
	int in_i = randomneuron(g, true, false);
	int in_o = randomneuron(g, false, true);
	g->addgene(in_i, in_o, 4*randfloat() - 2, newinnov());
}

void newbias(Genome* g)
{
	int in_o = randomneuron(g, false, true);
	g->addgene(0, in_o, 4*randfloat() - 2, newinnov());
}

void newnode(Genome* g)
{
	if (g->ngenes == 0)
		return;
	int gn = randint(0, g->ngenes-1);
	
	int nn = g->n_input + g->n_output + g->n_hidden + 1;
	g->n_hidden++;
	
	g->addgene(g->genes[gn].in, nn, 1., newinnov());
	g->addgene(nn, g->genes[gn].out, g->genes[gn].weight, newinnov());
	g->genes[gn].enabled = false;
}

void disablemutate(Genome* g)
{
	activationmutate(g, false);
}

void enablemutate(Genome* g)
{
	activationmutate(g, true);
}


void activationmutate(Genome* g, bool enable)
{
	Gene** gl = 0; int gn = 0;
	for (int i = 0; i < g->ngenes; i++)
	{
		if (g->genes[i].enabled != enable) listappend<Gene*>(gl, gn, &g->genes[i]);
	}
	if (gn == 0) return;
	int ind = randint(0, gn-1);
	gl[ind]->enabled = enable;
}


void doprob(Genome* g, float p, void(*act)(Genome*))
{
	while (p > 0)
	{
		if (randfloat() < p) act(g);
		p -= 1;
	}
}

Genome* mutate(Genome* oldg)
{
	Genome* ng = oldg->copy();
	
	doprob(ng, MutateConnectionsChance, mutateconnections);
	doprob(ng, LinkMutationChance, newlink);
	doprob(ng, BiasMutationChance, newbias);
	doprob(ng, NodeMutationChance, newnode);
	doprob(ng, DisableMutationChance, disablemutate);
	doprob(ng, EnableMutationChance, enablemutate);
	
	return ng;
}

Genome* cross(Genome* g1, Genome* g2)
{ // g1 higher fitness than g2
	Genome* child = g1->copy();
	for (int i = 0; i < child->ngenes; i++)
	{
		for (int j = 0; j < g2->ngenes; j++)
		{
			if (child->genes[i].innov == g2->genes[j].innov)
			{
				if (randint(0, 1))
					child->genes[i] = g2->genes[j];
				break;
			}
		}
	}
	return child;
}

void writegenome(Genome* g, const char* fname)
{
	std::ofstream fout;
	fout.open(fname);
	fout << g->n_input << " " << g->n_output << " " << g->n_hidden << std::endl;
	fout << g->ngenes << std::endl;
	for (int i = 0; i < g->ngenes; i++)
		fout << g->genes[i].in << " " << g->genes[i].out << " " << g->genes[i].weight << " " << g->genes[i].innov << " " << g->genes[i].enabled << std::endl;
	fout.close();
}

Genome* readgenome(const char* fname)
{
	std::ifstream fin;
	fin.open(fname);
	int n_i, n_o, n_h, ng;
	fin >> n_i >> n_o >> n_h >> ng;
	Genome* g = new Genome(n_i, n_o, n_h, ng);
	for (int i = 0; i < ng; i++)
		fin >> g->genes[i].in >> g->genes[i].out >> g->genes[i].weight >> g->genes[i].innov >> g->genes[i].enabled;
	fin.close();
	return g;
}



