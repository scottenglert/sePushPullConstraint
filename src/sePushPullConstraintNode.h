#ifndef _sePushPullConstraintNode
#define _sePushPullConstraintNode

#include <maya/MPxNode.h>
#include <maya/MTypeId.h> 
#include <maya/MStatus.h>
 
class sePushPullConstraint : public MPxNode
{
public:
						sePushPullConstraint();
	virtual				~sePushPullConstraint(); 

	virtual MStatus		compute( const MPlug& plug, MDataBlock& data );

	static  void*		creator();
	static  MStatus		initialize();
	
	static  MObject			constTransAttr;
	static  MObject			ctAttrX;
	static  MObject			ctAttrY;
	static  MObject			ctAttrZ;
	static  MObject			lastPositionAttr;
	static  MObject			lpAttrX;
	static  MObject			lpAttrY;
	static  MObject			lpAttrZ;
	static	MObject			distanceAttr;
	static	MObject			inTimeAttr;
	static	MObject			startFrameAttr;
	static	MObject			targetAttr;
	static	MObject			startPositionAttr;
	static  MObject			spAttrX;
	static  MObject			spAttrY;
	static  MObject			spAttrZ;
	static	MObject			pullAttr;
	static	MObject			pushAttr;
	static	MObject			constraintParentAttr;

	static	MTypeId		id;
};

#endif