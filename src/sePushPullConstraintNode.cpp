#include "sePushPullConstraintNode.h"

#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MGlobal.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MTime.h>
#include <maya/MVector.h>
#include <maya/MPoint.h>
#include <maya/MMatrix.h>

MObject sePushPullConstraint::constTransAttr;
MObject sePushPullConstraint::ctAttrX;
MObject sePushPullConstraint::ctAttrY;
MObject sePushPullConstraint::ctAttrZ;
MObject	sePushPullConstraint::lastPositionAttr;
MObject	sePushPullConstraint::lpAttrX;
MObject	sePushPullConstraint::lpAttrY;
MObject	sePushPullConstraint::lpAttrZ;
MObject	sePushPullConstraint::distanceAttr;
MObject	sePushPullConstraint::inTimeAttr;
MObject	sePushPullConstraint::startFrameAttr; 
MObject	sePushPullConstraint::targetAttr;
MObject	sePushPullConstraint::startPositionAttr;
MObject	sePushPullConstraint::spAttrX;
MObject	sePushPullConstraint::spAttrY;
MObject	sePushPullConstraint::spAttrZ;
MObject	sePushPullConstraint::pullAttr;
MObject	sePushPullConstraint::pushAttr;
MObject	sePushPullConstraint::constraintParentAttr;

sePushPullConstraint::sePushPullConstraint() {}
sePushPullConstraint::~sePushPullConstraint() {}

MStatus sePushPullConstraint::compute(const MPlug& plug, MDataBlock& data)
{
	MStatus status;

	// get the root plug if this is a child plug
	MPlug plugRoot;
	if (plug.isChild())
	{
		plugRoot = plug.parent();
	} else {
		plugRoot = plug;
	}

	// if the root plug is not the constraint translate attribute
	if (plugRoot != constTransAttr)
    {
        return MS::kUnknownParameter;
    }

	// get the current and start frame 
	MTime currentFrame = data.inputValue(inTimeAttr, &status).asTime();
	CHECK_MSTATUS_AND_RETURN_IT(status);

	double startFrame = data.inputValue(startFrameAttr, &status).asDouble();
	CHECK_MSTATUS_AND_RETURN_IT(status);

	// get the output attibutes
	MDataHandle outputHandle = data.outputValue(constTransAttr);

	if (currentFrame.value() < startFrame)
	{
		// copy the start position to the last and output plug, resetting the positions
		MVector startPos = data.inputValue(startPositionAttr, &status).asVector();
		CHECK_MSTATUS_AND_RETURN_IT(status);

		MDataHandle lastPosHandle = data.outputValue(lastPositionAttr, &status);
		CHECK_MSTATUS_AND_RETURN_IT(status);

		lastPosHandle.setMVector(startPos);
		outputHandle.setMVector(startPos);
		// we are done
		data.setClean(constTransAttr);
		data.setClean(lastPositionAttr);
		return MS::kSuccess;
	}

	// get the push / pull attribute
	bool isPullActive = data.inputValue(pullAttr).asBool();
	bool isPushActive = data.inputValue(pushAttr).asBool();

	if (! isPullActive && ! isPushActive){
		// if we don't push or pull then we don't do anything
		data.setClean(constTransAttr);
		return MS::kSuccess;
	}

	// use the last position we stored before
	MVector lp = data.inputValue(lastPositionAttr).asVector();
	MPoint lastPosition(lp);

	// get the target world matrix attribute
	MMatrix targetMat = data.inputValue(targetAttr).asMatrix();
	MPoint targetPos(targetMat[3][0], targetMat[3][1], targetMat[3][2]);

	// get the distance attribute
	double dist = data.inputValue(distanceAttr).asDouble();

	// get the constrained object parent matrix attribute
	MMatrix constraintParentMat = data.inputValue(constraintParentAttr).asMatrix();

	// this gives us the relative position from the target to the constrained
	MVector relativePos((lastPosition * constraintParentMat) - targetPos);

	// get the length to do some distance testing
	double currentDistance = relativePos.length();

	if ((isPullActive && currentDistance > dist) || (isPushActive && currentDistance < dist))
	{
		// so we have to calculate a new position
		relativePos.normalize();
		// gives us a new vector that is the proper distance away from the target
		relativePos *= dist;
		// add it to the target position so we know the new world space position
		MPoint newPosition = targetPos + relativePos;
		// convert it back to local space
		MVector localPos(newPosition * constraintParentMat.inverse());
		// set the new position to constraint plug
		outputHandle.setMVector(localPos);
		// set the last position plug
		MDataHandle lastPosOutHandle = data.outputValue(lastPositionAttr);
		lastPosOutHandle.setMVector(localPos);

		data.setClean(lastPositionAttr);
	}

	data.setClean(constTransAttr);

	return MS::kSuccess;
}

void* sePushPullConstraint::creator()
{
	return new sePushPullConstraint();
}

MStatus sePushPullConstraint::initialize()	
{
	MStatus status;
	// set the different attribute types we are going to use
	MFnNumericAttribute numericAttr;
	MFnUnitAttribute unitAttr;
    MFnMatrixAttribute matrixAttr;

	// NOTE createPoint does not work!!!!
	// create the output translation attribute
	ctAttrX = numericAttr.create("constraintTranslateX", "ctx", MFnNumericData::kDouble, 0.0);
    ctAttrY = numericAttr.create("constraintTranslateY", "cty", MFnNumericData::kDouble, 0.0);
    ctAttrZ = numericAttr.create("constraintTranslateZ", "ctz", MFnNumericData::kDouble, 0.0);
	constTransAttr = numericAttr.create("constraintTranslate", "ct", ctAttrX, ctAttrY, ctAttrZ);
    numericAttr.setWritable(false);
    addAttribute(constTransAttr);

	// create the in time attribute
	inTimeAttr = unitAttr.create("inTime", "it",  MFnUnitAttribute::kTime, 1.0);
	unitAttr.setStorable(false);
	unitAttr.setKeyable(false);
	unitAttr.setHidden(true);
    addAttribute(inTimeAttr);
    attributeAffects(inTimeAttr, constTransAttr);
    
	// create the start frame attribute
	startFrameAttr = numericAttr.create("startFrame", "stf", MFnNumericData::kDouble, 1.0);
    numericAttr.setKeyable(true);
    addAttribute(startFrameAttr);
    attributeAffects(startFrameAttr, constTransAttr);
    
	// create the distance attribute
	distanceAttr = numericAttr.create("distance", "dist", MFnNumericData::kDouble);
    numericAttr.setKeyable(true);
	numericAttr.setMin(0.0);
    addAttribute(distanceAttr);
    attributeAffects(distanceAttr, constTransAttr);
    
	// create the target world matrix attribute
	targetAttr = matrixAttr.create("targetWorldMatrix", "twm", MFnMatrixAttribute::kDouble);
	matrixAttr.setStorable(false);
    addAttribute(targetAttr);
    attributeAffects(targetAttr, constTransAttr);

	// create the constraint parent matrix attribute    
	constraintParentAttr = matrixAttr.create("constraintParentMatrix", "cpm", MFnMatrixAttribute::kDouble);
    matrixAttr.setStorable(false);
	addAttribute(constraintParentAttr);
    attributeAffects(constraintParentAttr, constTransAttr);
    
	// create the start position attribute
	spAttrX = numericAttr.create("startPositionX", "spx", MFnNumericData::kDouble, 0.0);
    spAttrY = numericAttr.create("startPositionY", "spy", MFnNumericData::kDouble, 0.0);
    spAttrZ = numericAttr.create("startPositionZ", "spz", MFnNumericData::kDouble, 0.0);    
	startPositionAttr = numericAttr.create("startPosition", "sp", spAttrX, spAttrY, spAttrZ);
    numericAttr.setKeyable(true);
    addAttribute(startPositionAttr);
    attributeAffects(startPositionAttr, constTransAttr);

	// create the push bool attribute
	pushAttr = numericAttr.create("push", "psh", MFnNumericData::kBoolean);
    numericAttr.setKeyable(true);
	numericAttr.setDefault(true);
    addAttribute(pushAttr);
    attributeAffects(pushAttr, constTransAttr);
    
    // create the push bool attribute
	pullAttr = numericAttr.create("pull", "pll", MFnNumericData::kBoolean);
    numericAttr.setKeyable(true);
	numericAttr.setDefault(true);
    addAttribute(pullAttr);
    attributeAffects(pullAttr, constTransAttr);

	// create the last position attribute for internal uses
	lpAttrX = numericAttr.create("lastPositionX", "lpx", MFnNumericData::kDouble, 0.0);
    lpAttrY = numericAttr.create("lastPositionY", "lpy", MFnNumericData::kDouble, 0.0);
    lpAttrZ = numericAttr.create("lastPositionZ", "lpz", MFnNumericData::kDouble, 0.0);
	lastPositionAttr = numericAttr.create("lastPosition", "lp", lpAttrX, lpAttrY, lpAttrZ);
    numericAttr.setHidden(true);
    addAttribute(lastPositionAttr);

	return MS::kSuccess;
}