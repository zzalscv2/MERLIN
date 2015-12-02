/////////////////////////////////////////////////////////////////////////
//
// Merlin C++ Class Library for Charged Particle Accelerator Simulations
//
// Class library version 3 (2004)
//
// Copyright: see Merlin/copyright.txt
//
// Last CVS revision:
// $Date: 2006/03/20 13:42:54 $
// $Revision: 1.6 $
//
/////////////////////////////////////////////////////////////////////////

#ifndef AcceleratorComponent_h
#define AcceleratorComponent_h 1

#include "merlin_config.h"
#include <string>

// EMField
#include "AcceleratorModel/EMField.h"
// AcceleratorGeometry
#include "AcceleratorModel/AcceleratorGeometry.h"
// Aperture
#include "AcceleratorModel/Aperture.h"
// ModelElement
#include "AcceleratorModel/ModelElement.h"
// WakePotentials
#include "AcceleratorModel/WakePotentials.h"

class ComponentTracker;

using std::string;

/**
* An AcceleratorComponent represents any component which
* can be placed in an accelerator lattice. Typically, a
* lattice model is constructed as an ordered sequence of
* AcceleratorComponents. AcceleratorComponents can be
* associated with a geometry (an AcceleratorGeometry
* object) and a em field (an EMField Region object), which
* when taken together uniquely define the field properties
* for the component. Component "tracking" is supported via
* the funtion PrepareTracker(Tracker&), which sets up a
* Tracker object to track the component.
*/
class AcceleratorComponent : public ModelElement
{
public:
	virtual ~AcceleratorComponent ();

	/**
	* Returns the unique index for this class of accelerator
	* components.
	*/
	virtual int GetIndex () const;

	/**
	* Returns the geometry length of the component.
	*/
	double GetLength () const;

	/**
	* Returns a pointer to the this components geometry.
	* Returns NULL if no geometry is associated with this
	* component.
	*/
	const AcceleratorGeometry* GetGeometry () const;

	/**
	* Returns a pointer to this components field. A NULL
	* pointer is returned if the component has no field.
	*/
	const EMField* GetEMField () const;

	/**
	* Returns a pointer to the aperture of this element
	*/
	Aperture* GetAperture () const;

	/**
	* Sets the aperture of this element
	* @param ap A pointer to an aperture class to associate with this component
	*/
	void SetAperture (Aperture* ap);

	/**
	* Returns the wake potentials associated with this cavity.
	*/
	WakePotentials* GetWakePotentials () const;

	/**
	* Sets the wake potentials associated with this cavity.
	* @param wp A pointer to a wakepotential class to associate with this component
	*/
	void SetWakePotentials (WakePotentials* wp);

	/**
	* Primary tracking interface. Prepares the specified
	* Tracker object for tracking this component.
	* @param aTracker The tracker to prepare
	*/
	virtual void PrepareTracker (ComponentTracker& aTracker);

	/**
	* Rotates the component 180 degrees about its local Y axis.
	*/
	virtual void RotateY180 () = 0;

	/**
	* Set the uniques beamline index for this frame
	* @param n The beamline index
	*/
	void SetBeamlineIndex(size_t n);

	/**
	* Get the uniques beamline index for this frame
	*/
	size_t GetBeamlineIndex() const;

	void AppendBeamlineIndecies(std::vector<size_t>&) const;

	/**
	* Returns the total number of distinct component types in
	* the system.
	*/
	static int TotalComponentNumber ();

	// Data Members for Class Attributes
	/**
	* Unique index for an Accelerator component.
	*/
	static const int ID;

	/**
	* Set the distance from the start of the lattice to the START of the element
	* @param position The position of the start of the lattice element
	*/
	void SetComponentLatticePosition(double position);

	/**
	* Get the distance from the start of the lattice to the START of the element
	*/
	double GetComponentLatticePosition() const;

	/*
	* Collimator ID for FLUKA output AV+HR 09.11.15
	* N.B. These values can only be set or got in Collimator
	*/
	void SetCollID (int n){Coll_ID = n;}
	int GetCollID() const {return Coll_ID;}


protected:

	/**
	* Protected constructors used by derived classes.
	*/
	explicit AcceleratorComponent (const string& aName = string());

	AcceleratorComponent (const string& aName, AcceleratorGeometry* aGeom, EMField* aField);

	/**
	* Used by derived classes to generate a unique index. All
	* derived classes should have a static member ID of type
	* IndexType which should be initialised as follows:
	*
	* IndexType component::ID = UniqueIndex();
	*/
	static int UniqueIndex ();

	/**
	* A pointer to the Electromagnetic field for this component
	*/
	EMField* itsField;

	/**
	* A pointer to the geometry field for this component
	*/
	AcceleratorGeometry* itsGeometry;

	/**
	* A pointer to the Aperture for this component
	*/
	Aperture* itsAperture;

	/**
	* A pointer to the Wake potentials for this component
	*/
	WakePotentials* itsWakes;

	/**
	* The position of this element relative to a user defined location
	* (the start of the user generated lattice)
	*/
	double position;

	/**
	* beamline index associated with this component
	*/
	size_t blI; 

	/*
	* Collimator ID for FLUKA output AV+HR 09.11.15
	* N.B. These values can only be set or got in Collimator
	*/
	int Coll_ID;


private:

	//Copy protection
	//AcceleratorComponent(const AcceleratorComponent& rhs);
	//AcceleratorComponent& operator=(const AcceleratorComponent& rhs);
};


inline AcceleratorComponent::AcceleratorComponent (const string& aName)
		: ModelElement(aName),itsField(0),itsGeometry(0),itsAperture(0),itsWakes(0),blI(0)
{}

inline AcceleratorComponent::AcceleratorComponent (const string& aName, AcceleratorGeometry* aGeom, EMField* aField)
		: ModelElement(aName),itsField(aField),itsGeometry(aGeom),itsAperture(0),itsWakes(0),blI(0)
{}

inline const AcceleratorGeometry* AcceleratorComponent::GetGeometry () const
{
	return itsGeometry;
}

inline const EMField* AcceleratorComponent::GetEMField () const
{
	return itsField;
}

inline  Aperture* AcceleratorComponent::GetAperture () const
{
	return itsAperture;
}

inline void AcceleratorComponent::SetAperture (Aperture* ap)
{
	itsAperture = ap;
}

inline WakePotentials* AcceleratorComponent::GetWakePotentials () const
{
	return itsWakes;
}

inline void AcceleratorComponent::SetWakePotentials (WakePotentials* wp)
{
	itsWakes=wp;

}

inline int AcceleratorComponent::TotalComponentNumber ()
{
	return 0;
}

inline void AcceleratorComponent::SetBeamlineIndex(size_t n)
{
	blI = n;
}

inline size_t AcceleratorComponent::GetBeamlineIndex() const
{
	return blI;
}

inline void AcceleratorComponent::AppendBeamlineIndecies(std::vector<size_t>& ivec) const
{
	ivec.push_back(blI);
}

inline void AcceleratorComponent::SetComponentLatticePosition(double distance)
{
	position = distance;
}


inline double AcceleratorComponent::GetComponentLatticePosition() const
{
	return position;
}
#endif
