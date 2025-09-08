#pragma once

#include <Unknwn.h>

class please
{
public:
	static void doit(IUnknown* iunk_state);

	//// by Xiaodong Liang March 10th
	//// get the primitives of the model
	static long _geometriescount;
	static long _fragscount;
	static long _primitivescount;

	//static void dumpFragments(raw::InwNodeFragsCollPtr fragments);
	static void doit_primitive(IUnknown* iunk_state);
	static void walkNode(IUnknown* iunk_node, bool bFoundFirst = false);
};

