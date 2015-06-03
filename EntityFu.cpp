///
/// [EntityFu](https://github.com/NatWeiss/EntityFu)
/// A simple, fast entity component system written in C++.
/// Under the MIT license.
///

#include "EntityFu.h"
using namespace std;

/// Turn this on to have a faster yet riskier ECS.
#define kTrustPointers 0

/// Auto-define a log method.
#ifndef Log
	#include <stdio.h>
	#define Log(...) {printf(__VA_ARGS__); printf("\n");}
#endif

/// Auto-define an assert method.
#ifndef Assert2
	#define Assert2(...) do {} while (0)
#endif

/// Turn this to 1 or 2 to debug the ECS.
/// 1 == log creation, 2 == log creation and deletion.
static int verbosity = 0;

bool* Entity::entities = nullptr;
Entity::Component*** Entity::components = nullptr;
vector<Eid>* Entity::componentEids = nullptr;

void Entity::alloc()
{
	if (components != nullptr)
		return;
	if (verbosity > 0)
		{Log("Allocing entities");}

	// allocate entities
	entities = new bool[kMaxEntities];
	for (Eid eid = 0; eid < kMaxEntities; ++eid)
		entities[eid] = false;

	// allocate components
	auto max = Component::numCids;
	components = new Component**[max];
	componentEids = new vector<Eid>[Component::numCids];
	for (Cid cid = 0; cid < max; cid++)
	{
		// allocate component array
		components[cid] = new Component*[kMaxEntities];
		
		// zero component pointers
		for (Eid eid = 0; eid < kMaxEntities; eid++)
			components[cid][eid] = nullptr;
	}
}

void Entity::dealloc()
{
	if (verbosity > 0)
		{Log("Deallocing entities");}
	
	if (components != nullptr)
	{
		Entity::destroyAll();
		for (Cid cid = 0; cid < Component::numCids; cid++)
			if (components[cid] != nullptr)
				delete [] components[cid];
		delete [] components;
	}

	if (componentEids != nullptr)
		delete [] componentEids;

	if (entities != nullptr)
		delete [] entities;
	
	entities = nullptr;
	components = nullptr;
	componentEids = nullptr;
}

Eid Entity::create()
{
	// auto allocate
	Entity::alloc();
	
	Eid eid = 1;
	for (; eid < kMaxEntities && entities[eid]; ++eid)
	{
	}

	if (eid < 1 || eid >= kMaxEntities)
		{Log("Maximum number of entities reached!");} // this should probably be an assertion
	else
		entities[eid] = true;

	if (verbosity > 0)
		{Log("Entity %u created", eid);}
	
	return eid;
}

void Entity::destroyNow(Eid eid)
{
	if (eid == 0)
		return;
	if (verbosity > 0)
		{Log("Entity %u being destroyed", eid);}

	for (Cid cid = 0; cid < Component::numCids; cid++)
		Entity::removeComponent(cid, eid);
	entities[eid] = false;
}

void Entity::destroyAll()
{
	for (Eid eid = 1; eid < kMaxEntities; ++eid)
		if (entities[eid])
			Entity::destroyNow(eid);
}

void Entity::addComponent(Cid cid, Eid eid, Component* c)
{
	if (c == nullptr)
		return;
	if (eid >= kMaxEntities || !entities[eid] || cid >= Component::numCids)
	{
		Assert2(false, "Invalid eid %u or cid %u", eid, cid);
		return;
	}
	if (verbosity > 0)
	{
		Log("");
		Entity::log(cid);
		Log("Adding component cid %u eid %u (%x)", cid, eid, (int)(long)c);
	}
	
	// if component already added, delete old one
	if (components[cid][eid] != nullptr)
		Entity::removeComponent(cid, eid);
	
	// pointers to components are stored in the map
	// (components must be allocated with new, not stack objects)
	components[cid][eid] = c;
	
	// store component eids
	componentEids[cid].push_back(eid);

	if (verbosity > 0)
		Entity::log(cid);
}

void Entity::removeComponent(Cid cid, Eid eid)
{
	if (eid >= kMaxEntities || !entities[eid] || cid >= Component::numCids)
	{
		Assert2(false, "Invalid eid %u or cid %u", eid, cid);
		return;
	}

	// get pointer
	auto ptr = components[cid][eid];
	if (ptr == nullptr)
		return;

	if (verbosity > 1)
	{
		Log("");
		Entity::log(cid);
		Log("Removing component cid %u eid %u (%x)", cid, eid, (int)(long)ptr);
	}

	// pointers to components are deleted
	delete ptr;
	
	// erase the component pointer
	components[cid][eid] = nullptr;

	// update component eids
	auto& eids = componentEids[cid];
	auto it = find(eids.begin(), eids.end(), eid);
	if (it != eids.end())
		it = eids.erase(it);
	
	if (verbosity > 1)
		Entity::log(cid);
}

Entity::Component* Entity::getComponent(Cid cid, Eid eid)
{
#if (kTrustPointers == 0)
	if (eid < kMaxEntities && cid < Component::numCids)
	{
#endif
		return components[cid][eid];

#if (kTrustPointers == 0)
	}
	return nullptr;
#endif
}

const vector<Eid>& Entity::getAll(Cid cid)
{
	if (cid < Component::numCids)
		return componentEids[cid];
	static vector<Eid> blankEids;
	return blankEids;
}

unsigned Entity::count()
{
	int ret = 0;
	for (Eid eid = 1; eid < kMaxEntities; ++eid)
		if (entities[eid])
			++ret;
	return ret;
}

unsigned Entity::count(Cid cid)
{
	return (unsigned)Entity::getAll(cid).size();
}

void Entity::log(Cid cid)
{
	auto n = Entity::count(cid);
	auto& eids = Entity::getAll(cid);
	if (eids.size() > 0)
		{Log("Cid %u has %d entities ranging from %u to %u", cid, n, eids.front(), eids.back());}
}

void Entity::logAll()
{
	for (Cid cid = 0, max = Component::numCids; cid < max; cid++)
		Entity::log(cid);
}


