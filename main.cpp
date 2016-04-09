#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <chipmunk/chipmunk.h>
#include <chipmunk/chipmunk_private.h>

#include <iostream>
#include <fstream>
#include <vector>

#include "neural.h"


#define frame_time_count 500
#define ship_density 1e6
#define shell_density 1e5
#define shell_muzzle_vel 20
#define base_camera_move 300
#define base_camera_rotate 90

#define sv2 sf::Vector2f

#define nosev cpv(0.5, 3)

cpVect cpvrotate(cpVect v, float a)
{
	return cpvrotate(v, cpvforangle(a));
}
sv2 sv2rotate(sv2 v, float a)
{
	cpVect cp = cpvrotate(cpv(v.x, v.y), -a*PI/180);
	return sv2(cp.x, cp.y);
}

	
sf::Clock* wclock;
std::ofstream afout; 

float gtime()
{
	return wclock->getElapsedTime().asSeconds();
}




class Ship
{
	public:
	
	cpBody* body;
	cpShape* cpshape;
	
	sf::ConvexShape shape;
	
	float last_fired;
	
	float nose_angle;
	
	bool player;
	Network* brain;
	Ship* target;
	
	int score;
	
	Ship(int vn, cpVect* vl, cpVect pos, Genome* ng = 0)
	{
		float mass = cpAreaForPoly(vn, vl, 0) * ship_density;
		float moi = cpMomentForPoly(mass, vn, vl, cpv(0, 0), 0);
		body = cpBodyNew(mass, moi);
		
		cpshape = cpPolyShapeNew(body, vn, vl, cpTransformIdentity, 0);
		cpShapeSetFriction(cpshape, 0.9);

		cpVect centroid = cpCentroidForPoly(vn, vl);		

		shape.setPointCount(vn);
		for (int i = 0; i < vn; i++)
		{
			shape.setPoint(i, sf::Vector2f(vl[i].x, vl[i].y));
		}
		
		cpBodySetCenterOfGravity(body, centroid);
		cpBodySetPosition(body, pos-centroid);
		cpBodySetVelocity(body, cpv(0, 0));
		
		cpShapeSetCollisionType(cpshape, 1);
		cpShapeSetUserData(cpshape, this);
		
		
		last_fired = 0;
		
		nose_angle = PI/2;
		
		
		player = false;
		target = 0;
		score = 0;
		if (ng == 0)
		{
			Genome* braingenome = mutate(readgenome("shipmind.mind"));
			brain = braingenome->makenetwork();
			delete braingenome;
		}
		else
		{
			brain = ng->makenetwork();
		}
		
		score = 0;
	}	
};

class Shell
{
	public:
	
	cpBody* body;
	cpShape* cpshape;
	
	sf::ConvexShape shape;
	
	Shell(cpVect pos, cpVect vel, float angle)
	{
		cpVect vl[4] = {cpv(0, 0), cpv(0.1, 0), cpv(0.07, 0.3), cpv(0.03, 0.3)};
		int vn = sizeof(vl)/sizeof(cpVect);
		
		float mass = cpAreaForPoly(vn, vl, 0) * shell_density;
		float moi = cpMomentForPoly(mass, vn, vl, cpv(0, 0), 0);
		body = cpBodyNew(mass, moi);
		
		cpshape = cpPolyShapeNew(body, vn, vl, cpTransformIdentity, 0);
		cpShapeSetFriction(cpshape, 0.9);

		cpVect centroid = cpCentroidForPoly(vn, vl);		

		shape.setPointCount(vn);
		for (int i = 0; i < vn; i++)
		{
			shape.setPoint(i, sf::Vector2f(vl[i].x, vl[i].y));
		}
		
		cpBodySetCenterOfGravity(body, centroid);
		cpBodySetPosition(body, pos-centroid);
		cpBodySetVelocity(body, vel);
		cpBodySetAngle(body, angle);
		
		cpShapeSetCollisionType(cpshape, 2);
		cpShapeSetUserData(cpshape, this);
	}
	
};



static cpBool scorecollision(cpArbiter *arb, cpSpace *space, void *data)
{
	cpShape** a = new cpShape*;
	cpShape** b = new cpShape*;
	cpArbiterGetShapes(arb, a, b);
	
	Ship* s = 0;
	if (cpShapeGetCollisionType(*a) == 1)
		s = (Ship*)cpShapeGetUserData(*a);
	else
		s = (Ship*)cpShapeGetUserData(*b);
	
	s->score -= 1;
	s->target->score += 1;
	
	return cpTrue;
}



class Phys
{
	public:
	
	std::vector<Ship*> ship_list;
	std::vector<Shell*> shell_list;
	cpSpace* space;

	float last_time_updated;
	float dt;
	
	float* last_frame_deltas;
	
	int maininit()
	{
		ship_list.clear();
		space = cpSpaceNew();
		last_time_updated = gtime();
		dt = 1;
		
		cpCollisionHandler *handler = cpSpaceAddCollisionHandler(space, 1, 2);
		handler->beginFunc = scorecollision;
		
		last_frame_deltas = new float[frame_time_count];
		for (int i = 0; i < frame_time_count; i++) last_frame_deltas[i] = 1;
		
		return 0;
	}
	
	int init()
	{
		maininit();

		cpVect vl[4] = {cpv(0, 0), cpv(1, 0), cpv(0.7, 3), cpv(0.3, 3)};
		Ship* s = addship(sizeof(vl)/sizeof(cpVect), vl, cpv(0, 0));
		s->player = true;

		// cpBodySetVelocity(s->body, cpv(1, 1));
		cpBodySetAngularVelocity(s->body, 0);
					
		cpVect vl2[4] = {cpv(0, 0), cpv(1, 0), cpv(0.7, 3), cpv(0.3, 3)};
		Ship* s2 = addship(sizeof(vl2)/sizeof(cpVect), vl2, cpv(20, 0));
		cpBodySetAngularVelocity(s2->body, 0);
		
		s->target = s2;
		s2->target = s;
		
		return 0;
	}
	
	int init(Genome* g1)
	{
		maininit();
		
		cpVect vl[4] = {cpv(0, 0), cpv(1, 0), cpv(0.7, 3), cpv(0.3, 3)};
		Ship* s = addship(sizeof(vl)/sizeof(cpVect), vl, cpv(0, 0));
		s->player = true;
					
		cpVect vl2[4] = {cpv(0, 0), cpv(1, 0), cpv(0.7, 3), cpv(0.3, 3)};
		Ship* s2 = addship(sizeof(vl2)/sizeof(cpVect), vl2, cpv(20, 0), g1);
		
		s->target = s2;
		s2->target = s;
		
		return 0;
	}	
	
	int init(Genome* g1, Genome* g2)
	{
		maininit();
		
		cpVect vl[4] = {cpv(0, 0), cpv(1, 0), cpv(0.7, 3), cpv(0.3, 3)};
		Ship* s = addship(sizeof(vl)/sizeof(cpVect), vl, cpv(0, 0), g1);
					
		cpVect vl2[4] = {cpv(0, 0), cpv(1, 0), cpv(0.7, 3), cpv(0.3, 3)};
		Ship* s2 = addship(sizeof(vl2)/sizeof(cpVect), vl2, cpv(20, 0), g2);
		
		s->target = s2;
		s2->target = s;
		
		return 0;
	}
	
	int update()
	{
		float old_time = last_time_updated;
		last_time_updated = gtime();
		dt = last_time_updated - old_time;
		for (int i = 0; i < frame_time_count-1; i++) last_frame_deltas[i+1] = last_frame_deltas[i];
		last_frame_deltas[0] = dt;
		
		for (std::vector<Ship*>::iterator it = ship_list.begin(); it != ship_list.end(); ++it)
		{
			bool* keylist = 0;
			if ((*it)->player)
			{
				keylist = getkeylistfromkeys();
			}
			else
			{
				update_brain(*it);
				keylist = (*it)->brain->readout();			
			}
			
			applykeys(*it, keylist);
			delete keylist;
		}
		
		cpSpaceStep(space, dt);
		
		return 0;
	}	
	
	int update(float dt)
	{		
		last_time_updated += dt;
		
		for (std::vector<Ship*>::iterator it = ship_list.begin(); it != ship_list.end(); ++it)
		{
			bool* keylist = 0;
			update_brain(*it);
			keylist = (*it)->brain->readout();
			applykeys(*it, keylist);
			delete keylist;
		}
		
		cpSpaceStep(space, dt);
		
		return 0;
	}
	

	int update_brain(Ship* s)
	{
		float* inputs = new float[NINPUTS];
		
		float noseang = cpBodyGetAngle(s->body) + s->nose_angle;
		cpVect rpos = cpBodyGetPosition(s->target->body) - cpBodyGetPosition(s->body);
		
		inputs[0] = cpvlength(rpos);
		inputs[1] = restrictangle(cpvtoangle(rpos)-noseang);
		
		cpVect rvel = cpBodyGetVelocity(s->target->body) - cpBodyGetVelocity(s->body);
		
		inputs[2] = (rpos.x * rvel.x + rpos.y * rvel.y)/inputs[0];
		inputs[3] = (rpos.x * rvel.y - rpos.y * rvel.x)/(inputs[0]*inputs[0]) - cpBodyGetAngularVelocity(s->body);
		
		inputs[4] = restrictangle(cpBodyGetAngle(s->target->body) + s->target->nose_angle - noseang);
		inputs[5] = cpBodyGetAngularVelocity(s->target->body);
		
		s->brain->update(inputs);
		
		delete inputs;		
		return 0;
	}
	
	
	bool* getkeylistfromkeys()
	{
		bool* ret = new bool[7];
		
		ret[0] = sf::Keyboard::isKeyPressed(sf::Keyboard::W);
		ret[1] = sf::Keyboard::isKeyPressed(sf::Keyboard::S);
		ret[2] = sf::Keyboard::isKeyPressed(sf::Keyboard::A);
		ret[3] = sf::Keyboard::isKeyPressed(sf::Keyboard::D);
		ret[4] = sf::Keyboard::isKeyPressed(sf::Keyboard::Q);
		ret[5] = sf::Keyboard::isKeyPressed(sf::Keyboard::E);
		ret[6] = sf::Keyboard::isKeyPressed(sf::Keyboard::Space);
		
		return ret;
	}
	
	
	int applykeys(Ship* s, bool* keylist)
	{
		if (keylist[0])
			cpBodyApplyForceAtLocalPoint(s->body, cpv(0, 1e7), cpv(0.5, 0));
		if (keylist[1])
			cpBodyApplyForceAtLocalPoint(s->body, cpv(0, -1e6), cpv(0.5, 3));
		if (keylist[2])
		{
			cpBodyApplyForceAtLocalPoint(s->body, cpv(1e6, 0), cpv(0, 0));
			cpBodyApplyForceAtLocalPoint(s->body, cpv(-1e6, 0), cpv(1, 3));
		}
		if (keylist[3])
		{
			cpBodyApplyForceAtLocalPoint(s->body, cpv(1e6, 0), cpv(0, 3));
			cpBodyApplyForceAtLocalPoint(s->body, cpv(-1e6, 0), cpv(1, 0));
		}
		if (keylist[4])
		{
			cpBodyApplyForceAtLocalPoint(s->body, cpv(-1e6, 0), cpv(1, 3));
			cpBodyApplyForceAtLocalPoint(s->body, cpv(-1e6, 0), cpv(1, 0));
		}
		if (keylist[5])
		{
			cpBodyApplyForceAtLocalPoint(s->body, cpv(1e6, 0), cpv(0, 3));
			cpBodyApplyForceAtLocalPoint(s->body, cpv(1e6, 0), cpv(0, 0));
		}

		
		if (keylist[6] && last_time_updated - s->last_fired > 1)
		{			
			cpVect tip = cpBodyLocalToWorld(s->body, nosev + cpv(0, 0.1));
			cpVect base = cpBodyLocalToWorld(s->body, cpv(0.5, 0));
			
			cpVect newvel = cpvnormalize(tip - base) * shell_muzzle_vel + cpBodyGetVelocityAtLocalPoint(s->body, nosev);
			Shell* newshell = addshell(tip, newvel, cpBodyGetAngle(s->body));
			cpBodyApplyImpulseAtLocalPoint(s->body, cpv(0, -1)*cpBodyGetMass(newshell->body)*shell_muzzle_vel, cpv(0.5, 3));
			
			s->last_fired = last_time_updated;
		}
		
		
		return 0;
	}
	
	
	
	Ship* addship(int vn, cpVect* vl, cpVect pos, Genome* g = 0)
	{
		Ship* s = new Ship(vn, vl, pos, g);
		ship_list.push_back(s);
		cpSpaceAddBody(space, s->body);
		cpSpaceAddShape(space, s->cpshape);
		
		return *ship_list.rbegin();
	}	
	
	Shell* addshell(cpVect pos, cpVect vel, float angle)
	{
		Shell* s = new Shell(pos, vel, angle);
		shell_list.push_back(s);
		cpSpaceAddBody(space, s->body);
		cpSpaceAddShape(space, s->cpshape);
		
		return *shell_list.rbegin();
	}
	
	
	
};

class Rend
{
	public:
	
	sf::RenderWindow* window;
	sf::View* view;
	sf::View* noview;
	sf::Font arial;
	
	float scrw, scrh;
	
	float mouse_wheel;

	
	int init()
	{
		scrw = 800; scrh = 600;
		mouse_wheel = 0;
		
		window = new sf::RenderWindow(sf::VideoMode(scrw, scrh), "thing");
		window->setVerticalSyncEnabled(true);
		
		view = new sf::View(sf::FloatRect(-40, 30, 80, -60));
		noview = new sf::View(sf::FloatRect(0, 0, scrw, scrh));
		window->setView(*view);
		
		arial.loadFromFile("arial.ttf");
		
		return 0;
	}
	
	int render(Phys* phys)
	{
        window->clear(sf::Color::Black);
		
		sf::Vector2f viewmove(0, 0);
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) viewmove.x -= base_camera_move;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) viewmove.x += base_camera_move;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) viewmove.y += base_camera_move;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) viewmove.y -= base_camera_move;
		viewmove *= (phys->dt * view->getSize().x / scrw);
		viewmove = sv2rotate(viewmove, -view->getRotation());
		view->move(viewmove);
		
		view->zoom(pow(1.2, -mouse_wheel));
		mouse_wheel = 0;

		float viewrotate = 0;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::LBracket)) viewrotate += base_camera_rotate;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::RBracket)) viewrotate -= base_camera_rotate;
		viewrotate *= phys->dt;
		view->rotate(viewrotate);
		
		window->setView(*view);
		
		
		
		for (std::vector<Ship*>::iterator its = phys->ship_list.begin(); its != phys->ship_list.end(); ++its)
		{
			sf::ConvexShape& shape = (*its)->shape;
			cpVect pos = cpBodyGetPosition((*its)->body);
			cpFloat angle = cpBodyGetAngle((*its)->body);
			shape.setPosition(pos.x, pos.y);
			shape.setRotation(180 / PI * angle);
			window->draw(shape);
		}		
		for (std::vector<Shell*>::iterator its = phys->shell_list.begin(); its != phys->shell_list.end(); ++its)
		{
			sf::ConvexShape& shape = (*its)->shape;
			cpVect pos = cpBodyGetPosition((*its)->body);
			cpFloat angle = cpBodyGetAngle((*its)->body);
			shape.setPosition(pos.x, pos.y);
			shape.setRotation(180 / PI * angle);
			window->draw(shape);
		}
		
		char* str = new char[30];
		float acc = 0;
		for (int i = 0; i < frame_time_count; i++) acc += phys->last_frame_deltas[i];
		acc /= frame_time_count;
		sprintf(str, "%.1f", 1./acc);
		drawtext(str, 0.02, 0.98, 10);	
		
		int sc = (*phys->ship_list.begin())->score;
		sprintf(str, "%d", sc);
		drawtext(str, 0.98, 0.98, 10);
		
		delete str;
		
		
        window->display();
		
		return 0;
	}
	
	int drawtext(char* textstr, float xpos, float ypos, int size)
	{
		window->setView(*noview);
		
		sf::Text text;
		text.setFont(arial);
		text.setString(textstr);
		text.setCharacterSize(size);
		text.setColor(sf::Color::White);
		text.setPosition(sf::Vector2f(xpos*scrw, (1-ypos)*scrh));
		
		window->draw(text);
		
		window->setView(*view);
		
		return 0;
	}
	
	
};




struct Scores
{
	int s1, s2;
};
struct Score
{
	int n, s;
};
int scorecomp(const void* as1, const void* as2)
{
	Score* s1 = (Score*)as1;
	Score* s2 = (Score*)as2;
	
	if (s1->s < s2->s) return 1;
	if (s1->s == s2->s) return 0;
	if (s1->s > s2->s) return -1;
	return 0;
}

Scores evaluate(Genome* g1, Genome* g2, float t_max, float tstep)
{
	Phys phys; phys.init(g1, g2);
	int nframes = t_max/tstep;
    for (int i = 0; i < nframes; i++)
    {
		phys.update(tstep);		
    }
	
	int s1 = (*phys.ship_list.begin())->score;
	int s2 = (*phys.ship_list.rbegin())->score;
	
	return {s1, s2};
}



int openwindow(Genome* g1=0, Genome* g2=0)
{
	Phys phys; phys.init(g1, g2);
	Rend rend; rend.init();

    while (rend.window->isOpen())
    {
        sf::Event event;
        while (rend.window->pollEvent(event))
        {
			switch (event.type)
			{
				case sf::Event::Closed:
				{
					rend.window->close();
					afout.close();
					break;
				}
				case sf::Event::MouseWheelScrolled:
				{
					rend.mouse_wheel = event.mouseWheelScroll.delta;
					break;
				}
				default: break;
			}
        }
		
		phys.update();
		rend.render(&phys);
		
    }

    return 0;
}


int main()
{
	afout.open("out.txt");
	wclock = new sf::Clock;
	
	float t_max = 60;
	float tstep = 1./60.;
	
	int ngenerations = 5;
	int npop = 10;
	
	Genome* best;
	Genome* prev1, * prev2;
	prev1 = readgenome("shipmind.mind"); prev2 = readgenome("shipmind.mind");
	Genome** population = new Genome*[npop];
	for (int i = 0; i < npop; i++)
		population[i] = mutate(readgenome("shipmind.mind"));
	Score* scores = new Score[npop];
	
	for (int gn = 0; gn < ngenerations; gn++)
	{
		std::cout << gn << std::endl;
		
		for (int i = 0; i < npop; i++)
		{
			Scores s1 = evaluate(population[i], prev1, t_max, tstep);
			Scores s2 = evaluate(population[i], prev2, t_max, tstep);
			scores[i] = {i, s1.s1+s2.s1};
		}
		
		qsort(scores, npop, sizeof(Score), scorecomp);
		prev1 = population[scores[0].n];
		prev2 = population[scores[1].n];
		
		afout << std::endl <<  "Generation " << gn << std::endl;
		for (int i = 0; i < npop; i++)
		{
			afout << scores[i].s << std::endl;
		}
		
		for (int i = 0; i < npop/2; i++)
		{
			population[2*i] = mutate(prev1);
			population[2*i+1] = mutate(prev2);
		}
		
		best = prev1;
	}

	writegenome(best, "best.mind");
	return openwindow(best);
}


// int main()
// {
	// afout.open("out.txt");
	// wclock = new sf::Clock;
	
	// Genome* g1 = readgenome("best.mind");
	// Genome* g2 = readgenome("shipmind.mind");
	
	// return openwindow(g1, g2);
// }

