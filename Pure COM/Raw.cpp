//------------------------------------------------------------------
// NavisWorks Sample code
//------------------------------------------------------------------

// (C) Copyright 2018 by Autodesk Inc.

// Permission to use, copy, modify, and distribute this software in
// object code form for any purpose and without fee is hereby granted,
// provided that the above copyright notice appears in all copies and
// that both that copyright notice and the limited warranty and
// restricted rights notice below appear in all supporting
// documentation.

// AUTODESK PROVIDES THIS PROGRAM "AS IS" AND WITH ALL FAULTS.
// AUTODESK SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTY OF
// MERCHANTABILITY OR FITNESS FOR A PARTICULAR USE.  AUTODESK
// DOES NOT WARRANT THAT THE OPERATION OF THE PROGRAM WILL BE
// UNINTERRUPTED OR ERROR FREE.
//------------------------------------------------------------------

#include "Raw.h"

#include <atlbase.h>
#include <atlcom.h>
//#include <span>

#import "C:\Program Files\Autodesk\Navisworks Manage 2023\lcodieD.dll"  rename_namespace("raw")

using namespace System;

CComModule _Module;
long please::_geometriescount;
long please::_fragscount;
long please::_primitivescount;

/*
ref class dateClass {
	public:
		static System::DateTime stTime; 
		static System::IO::StreamWriter^ outfile;
};
*/

class CSeeker : public ATL::CComObjectRoot,
				public IDispatchImpl<raw::InwSeekSelection>
{
public:
	BEGIN_COM_MAP(CSeeker)
		COM_INTERFACE_ENTRY(raw::InwSeekSelection)
	END_COM_MAP()

	STDMETHOD(raw_SelectNode)(/*[in]*/ struct raw::InwOaNode* node,
							/*[in]*/ struct raw::InwOaPath* path,
							/*[in,out]*/ VARIANT_BOOL* Add,
							/*[in,out]*/ VARIANT_BOOL* finished) 
	{
		return S_OK;
	}

	CSeeker() {}
};

void please::doit(IUnknown* iunk_state) 
{
	raw::InwOpState10Ptr state(iunk_state);
	raw::InwOpSelectionPtr selection=state->ObjectFactory(raw::eObjectType_nwOpSelection);

	CComObject<CSeeker> *cseeker;

	HRESULT HR=CComObject<CSeeker>::CreateInstance(&cseeker);
	raw::InwSeekSelectionPtr seeker=cseeker->GetUnknown();//???

	state->SeekSelection(selection,seeker);
}

//// by Xiaodong Liang March 10th
//// get the primitives of the model

// callback class
class CallbackGeomClass:public ATL::CComObjectRoot,
	public IDispatchImpl<raw::InwSimplePrimitivesCB>
{
	public:
		BEGIN_COM_MAP(CallbackGeomClass)
			COM_INTERFACE_ENTRY(raw::InwSimplePrimitivesCB)
		END_COM_MAP()

	float x, y, z; // temp storage for coords

	void GetPoint(struct raw::InwSimpleVertex* v)
	{
		// do your work
		auto coords = v->Getcoord();
		auto array = coords.parray;
		//System::Diagnostics::Debug::Assert(array->cDims == 1);
		//System::Diagnostics::Debug::Assert(array->cbElements == 4);
		//System::Diagnostics::Debug::Assert(array->rgsabound->cElements == 3);
		float* vs = (float*)(array->pvData);
		x = vs[0];
		y = vs[1];
		z = vs[2];
	}

	STDMETHOD(raw_Triangle)(/*[in]*/ struct raw::InwSimpleVertex* v1,
							/*[in]*/ struct raw::InwSimpleVertex* v2,
							/*[in]*/ struct raw::InwSimpleVertex* v3
							)
	{
		please::_primitivescount++; // one more primitive

		GetPoint(v1);
		GetPoint(v2);
		GetPoint(v3);

		return S_OK;
	}

	STDMETHOD(raw_Line)(/*[in]*/ struct raw::InwSimpleVertex* v1,
						/*[in]*/ struct raw::InwSimpleVertex* v2 
					)
	{
		please::_primitivescount++; // one more primitive

		GetPoint(v1);
		GetPoint(v2);

		return S_OK;
	}

	STDMETHOD(raw_Point)(/*[in]*/ struct raw::InwSimpleVertex* v1) 
	{
		please::_primitivescount++; // one more primitive

		GetPoint(v1);

		return S_OK;
	}

	STDMETHOD(raw_SnapPoint)(/*[in]*/ struct raw::InwSimpleVertex* v1)
	{
		please::_primitivescount++; // one more primitive

		GetPoint(v1);

		return S_OK;
	}

	CallbackGeomClass() {}
};


//static void DumpFragments(raw::InwNodeFragsCollPtr fragments)
static void DumpFragments(raw::InwNodeFragsColl* fragments)
{
	long fragsCount = fragments->Count;

#ifdef xxDEBUG
	System::Diagnostics::Debug::WriteLine("frags count:" + fragsCount.ToString());
#endif

	_variant_t _fragIndex = _variant_t(1);
	for (long fragindex = 1; fragindex <= fragsCount; fragindex++)
	{
		CComObject<CallbackGeomClass>* callbkListener;
		HRESULT HR = CComObject<CallbackGeomClass>::CreateInstance(&callbkListener);

		_fragIndex.intVal = fragindex;
		raw::InwOaFragment3Ptr frag = fragments->GetItem(&_fragIndex); // _variant_t(fragindex));
		//VARIANT varGeometry;
		//VariantInit(&varGeometry);
		//HRESULT hr = frag->get_Geometry(&varGeometry);
		//if (FAILED(hr))
		//{
		//	//Debug::WriteLine(L"get_Geometry failed with err: " + hr);
		//}

		//if (!FAILED(hr))
		frag->GenerateSimplePrimitives(raw::nwEVertexProperty::eNORMAL,
			callbkListener);

		//please::_fragscount++;
	}

	please::_fragscount += fragsCount;
}

// walk through the model and get the primitives
void please::walkNode(IUnknown* iunk_node, bool bFoundFirst)
{
	raw::InwOaNodePtr node(iunk_node);

	// If this is a group node then recurse into the structure
	if (node->IsGroup)
	{
		raw::InwOaGroupPtr group = (raw::InwOaGroupPtr)node;
		long subNodesCount = group->Children()->GetCount();
		for(long subNodeIndex = 1; subNodeIndex <= subNodesCount ; subNodeIndex++)
		{
			if ((!bFoundFirst) && (subNodesCount > 1))
			{
				bFoundFirst = true;
			}
			
			raw::InwOaNodePtr newNode = group->Children()->GetItem(_variant_t(subNodeIndex));
			walkNode(newNode, bFoundFirst);
		}
	}
	else if (node->IsGeometry)
	{
		please::_geometriescount++; // one more node

		raw::InwNodeFragsCollPtr fragments = node->Fragments();
		DumpFragments(fragments);
		
#ifdef xxDEBUG
		System::DateTime nowTime = System::DateTime::Now;
		System::TimeSpan span = nowTime.Subtract(dateClass::stTime);
		//dateClass::span = dateClass::nowTime.Subtract()
		System::Diagnostics::Debug::WriteLine(please::_geonodecount +  " nodes done:" + span.TotalMilliseconds.ToString());
#endif
	}
}

// do primitive
void please::doit_primitive(IUnknown* iunk_state) 
{
	please::_geometriescount = 0;
	please::_fragscount = 0;
	please::_primitivescount = 0;

	raw::InwOpState10Ptr state(iunk_state);
	raw::InwOpSelectionPtr opSelection = state->CurrentSelection;
	raw::InwSelectionPathsCollPtr paths = opSelection->Paths();

	int pc = paths->Count;
	if (pc < 1)
		return;
	
	DateTime stTime = DateTime::Now;

	for (int i = 1; i <= pc; i++)
	{
		//_path.intVal = i;
		_variant_t _pathIndex = _variant_t(i);
		_variant_t _path = paths->GetItem(&_pathIndex); // 1 based
		//raw::InwOaPathPtr path = (IUnknown*)_path;
		//raw::InwNodeFragsCollPtr fragments = path->Fragments();
		raw::InwOaPath* path = (raw::InwOaPath*)_path.punkVal;
		//raw::InwNodeFragsColl* fragments = path->Fragments();
		struct raw::InwNodeFragsColl* fragments = 0;
		path->raw_Fragments(&fragments);

		if (fragments->Count < 1)
			continue;

		please::_geometriescount++;
		DumpFragments(fragments);
	}

	//dateClass::outfile = gcnew System::IO::StreamWriter("c:\\temp\\dump.rtf");

	//raw::InwOaPartitionPtr oP = state->CurrentPartition;
	//walkNode(oP, false);

	//dateClass::outfile->Close();

	System::DateTime nowTime = System::DateTime::Now;
	System::TimeSpan span = nowTime.Subtract(stTime);
	//dateClass::span = dateClass::nowTime.Subtract();
#ifdef DEBUG
	System::Diagnostics::Debug::WriteLine(span.TotalMilliseconds.ToString());
#endif
	System::Windows::Forms::MessageBox::Show(span.TotalMilliseconds.ToString() + " ms, " + 
											please::_geometriescount + " geometries, " + 
											please::_fragscount + " fragments, " +
											please::_primitivescount + " primitives");
}

