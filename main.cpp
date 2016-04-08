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
	
	Ship(int vn, cpVect* vl, cpVect pos)
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
		
		last_fired = 0;
		
		nose_angle = PI/2;
		
		
		player = false;
		brain = shipmind2();
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
	}
	
};



class Phys
{
	public:
	
	std::vector<Ship> ship_list;
	std::vector<Shell> shell_list;
	cpSpace* space;

	float last_time_updated;
	float dt;
	
	float* last_frame_deltas;
	
	
	int init()
	{
		ship_list.clear();
		space = cpSpaceNew();
		last_time_updated = gtime();
		dt = 1;
		
		last_frame_deltas = new float[frame_time_count];
		for (int i = 0; i < frame_time_count; i++) last_frame_deltas[i] = 1;

			cpVect vl[4] = {cpv(0, 0), cpv(1, 0), cpv(0.7, 3), cpv(0.3, 3)};
			Ship* s = addship(sizeof(vl)/sizeof(cpVect), vl, cpv(0, 0));
			s->player = true;

			// cpBodySetVelocity(s->body, cpv(1, 1));
			cpBodySetAngularVelocity(s->body, 0);
						
			cpVect vl2[4] = {cpv(0, 0), cpv(1, 0), cpv(0.7, 3), cpv(0.3, 3)};
			Ship* s2 = addship(sizeof(vl2)/sizeof(cpVect), vl2, cpv(3, 0));
			cpBodySetAngularVelocity(s2->body, 0);
		
		return 0;
	}
	
	int update()
	{
		float old_time = last_time_updated;
		last_time_updated = gtime();
		dt = last_time_updated - old_time;
		for (int i = 0; i < frame_time_count-1; i++) last_frame_deltas[i+1] = last_frame_deltas[i];
		last_frame_deltas[0] = dt;
		
		for (std::vector<Ship>::iterator it = ship_list.begin(); it != ship_list.end(); ++it)
		{
			bool* keylist = 0;
			if (it->player)
			{
				keylist = getkeylistfromkeys();
			}
			else
			{
				update_brain(&*it);
				keylist = it->brain->readout();
				
				
					float* inl = it->brain->readin();
					afout << "in:" << std::endl;
					for (int i = 0; i < NINPUTS; i++)
						afout << inl[i] << std::endl;
					float* outl = it->brain->readoutnodes();
					afout << "out:" << std::endl;
					for (int i = 0; i < NOUTPUTS; i++)
						afout << outl[i] << std::endl;
					afout << std::endl;
					
				
			}
			
			applykeys(&*it, keylist);
			delete keylist;
		}
		
		cpSpaceStep(space, dt);
		
		return 0;
	}
	
	int update_brain(Ship* s)
	{
		float* inputs = new float[NINPUTS];
		inputs[0] = 0;
		inputs[1] = cpBodyGetAngularVelocity(s->body);
		
		Ship* target = &*ship_list.begin();
		cpVect rpos = cpBodyGetPosition(target->body) - cpBodyGetPosition(s->body);
		float noseang = cpBodyGetAngle(s->body) + s->nose_angle;
		
		inputs[2] = cpvlength(rpos);
		inputs[3] = restrictangle(cpvtoangle(rpos) - noseang);
		inputs[4] = restrictangle(cpBodyGetAngle(target->body) + target->nose_angle - noseang);
		inputs [5] = cpBodyGetAngularVelocity(target->body);
		
		s->brain->update(inputs);
		
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
			cpVect tip = cpBodyLocalToWorld(s->body, cpv(0.5, 3.1));
			cpVect base = cpBodyLocalToWorld(s->body, cpv(0.5, 0));
			
			cpVect newvel = cpvnormalize(tip - base) * shell_muzzle_vel + cpBodyGetVelocityAtLocalPoint(s->body, cpv(0.5, 3));
			Shell* newshell = addshell(tip, newvel, cpBodyGetAngle(s->body));
			cpBodyApplyImpulseAtLocalPoint(s->body, cpv(0, -1)*cpBodyGetMass(newshell->body)*shell_muzzle_vel, cpv(0.5, 3));
			
			s->last_fired = last_time_updated;
		}
		
		
		return 0;
	}
	
	
	
	Ship* addship(int vn, cpVect* vl, cpVect pos)
	{
		Ship s(vn, vl, pos);
		ship_list.push_back(s);
		cpSpaceAddBody(space, s.body);
		cpSpaceAddShape(space, s.cpshape);
		
		return &*ship_list.rbegin();
	}	
	
	Shell* addshell(cpVect pos, cpVect vel, float angle)
	{
		Shell s(pos, vel, angle);
		shell_list.push_back(s);
		cpSpaceAddBody(space, s.body);
		cpSpaceAddShape(space, s.cpshape);
		
		return &*shell_list.rbegin();
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
		
		
		
		for (std::vector<Ship>::iterator its = phys->ship_list.begin(); its != phys->ship_list.end(); ++its)
		{
			sf::ConvexShape& shape = its->shape;
			cpVect pos = cpBodyGetPosition(its->body);
			cpFloat angle = cpBodyGetAngle(its->body);
			shape.setPosition(pos.x, pos.y);
			shape.setRotation(180 / PI * angle);
			window->draw(shape);
		}		
		for (std::vector<Shell>::iterator its = phys->shell_list.begin(); its != phys->shell_list.end(); ++its)
		{
			sf::ConvexShape& shape = its->shape;
			cpVect pos = cpBodyGetPosition(its->body);
			cpFloat angle = cpBodyGetAngle(its->body);
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



int main()
{
	afout.open("out.txt");
	
	wclock = new sf::Clock;
	
	Phys phys; phys.init();
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




