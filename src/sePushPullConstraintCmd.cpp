#include "sePushPullConstraintCmd.h"
#include <maya/MArgDatabase.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MDagPath.h>
#include <maya/MGlobal.h>
#include <maya/MFnTransform.h>
#include <maya/MPlug.h>
#include <maya/MVector.h>
#include <maya/MString.h>
#include <maya/MArgList.h>
#include <vector>
#include <stdlib.h>

sePushPullConstraintCmd::sePushPullConstraintCmd() {}
sePushPullConstraintCmd::~sePushPullConstraintCmd() {}

const char *nodeNameFlag = "-n", *nodeNameFlagLong = "-name";
const char *distanceFlag = "-d", *distanceFlagLong = "-distance";
const char *startFrameFlag = "-sf", *startFrameFlagLong = "-startFrame";
const char *startPositionFlag = "-sp", *startPositionFlagLong = "-startPosition";
const char *skipFlag = "-sk", *skipFlagLong = "-skip";

void* sePushPullConstraintCmd::creator()
{
	return new sePushPullConstraintCmd();
}

MStatus sePushPullConstraintCmd::doIt(const MArgList& args)
{
	MStatus status;
	// setup the argument database
	MArgDatabase argData(syntax(), args);

	// get the extra objects that were passed in
	argData.getObjects(sList);

	int selNum = sList.length();
	if(selNum < 2)
	{
		MGlobal::displayError("Two transforms are required to create constraint.");
		return MS::kFailure;
	}

	// get the target object
	MDagPath targetDAG;
	MFnTransform targetTransFn;
	status = sList.getDagPath(0, targetDAG);
	if(!status)
	{
		MGlobal::displayError("Target object must be a dag object type. Unable to get path to object.");
		return status;
	}

	status = targetTransFn.setObject(targetDAG);
	if(!status)
	{
		MGlobal::displayError("Target object type invalid. You must choose a transform.");
		return status;
	}

	// get the constrained object
	MDagPath constrainedDAG;
	MFnTransform constrainedTransFn;
	status = sList.getDagPath(1, constrainedDAG);
	if(!status)
	{
		MGlobal::displayError("Constraint object must be a dag object type. Unable to get path to object.");
		return status;
	}

	status = constrainedTransFn.setObject(constrainedDAG);
	if(!status)
	{
		MGlobal::displayError("Constraint object type invalid. You must choose a transform.");
		return status;
	}

	unsigned int numSkips = argData.numberOfFlagUses(skipFlag);
	if(numSkips > 3){
		MGlobal::displayError("You can not have more than 3 skip flags.");
		return status;
	}

	// see if the name flag is set and if so, get the name
	if(argData.isFlagSet(nodeNameFlag))
	{
		argData.getFlagArgument(nodeNameFlag, 0, nodeName);
	} else {
		nodeName = "";
	}

	skipX = false;
	skipY = false;
	skipZ = false;

	for(unsigned int i = 0; i < numSkips; i++)
	{
		MArgList argList;
		status = argData.getFlagArgumentList(skipFlag, i, argList);
		CHECK_MSTATUS_AND_RETURN_IT(status);

		MString axis = argList.asString(0, &status);
		CHECK_MSTATUS_AND_RETURN_IT(status);

		if(axis == "x")
			skipX = true;
		if(axis == "y")
			skipY = true;
		if(axis == "z")
			skipZ = true;
	}

	// set the attributes
	// first set the distance
	if(argData.isFlagSet(distanceFlag))
	{
		argData.getFlagArgument(distanceFlag, 0, distanceValue);
	} else {
		// calculate the distance
		MVector targetPos = targetTransFn.getTranslation(MSpace::kWorld);
		MVector constrainedPos = constrainedTransFn.getTranslation(MSpace::kWorld);
		MVector localPos = targetPos - constrainedPos;
		distanceValue = localPos.length();	
	}

	// set the start frame
	if(argData.isFlagSet(startFrameFlag))
	{
		argData.getFlagArgument(startFrameFlag, 0, startFrame);
	} else {
		MGlobal::executeCommand("currentTime -q", startFrame);
	}

	// set the start position
	if(argData.isFlagSet(startPositionFlag))
	{
		argData.getFlagArgument(startPositionFlag, 0, startVector.x);
		argData.getFlagArgument(startPositionFlag, 1, startVector.y);
		argData.getFlagArgument(startPositionFlag, 2, startVector.z);
	} else {
		startVector = constrainedTransFn.getTranslation(MSpace::kTransform);
	}
	
	return redoIt();
}

MStatus sePushPullConstraintCmd::undoIt()
{
	MString deleteCmd = "delete ";
	deleteCmd += nodeName;
	MGlobal::executeCommand(deleteCmd);

	return MStatus::kSuccess;
}

MStatus sePushPullConstraintCmd::redoIt()
{
	MStatus status;

	MDagPath targetDAG;
	status = sList.getDagPath(0, targetDAG);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	MFnTransform targetTransFn;
	status = targetTransFn.setObject(targetDAG);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	MDagPath constrainedDAG;
	status = sList.getDagPath(1, constrainedDAG);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	MFnTransform constrainedTransFn;
	status = constrainedTransFn.setObject(constrainedDAG);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	// get the time node (don't know a better way but to filter the dg for the time type)
	MObject timeObject;
	for (MItDependencyNodes listIter(MFn::kTime); !listIter.isDone(); listIter.next())
	{
		timeObject = listIter.thisNode();
		break;
	}
	MFnDependencyNode timeNode(timeObject);

	// create the node and rename it
	MFnDependencyNode depNodeFn;
	if(nodeName == "")
	{
		depNodeFn.create("sePushPullConstraint", &status);
	} else {
		depNodeFn.create("sePushPullConstraint", nodeName, &status);
	}
	CHECK_MSTATUS_AND_RETURN_IT(status);
	nodeName = depNodeFn.name();

	// make the connections
	MPlug targetWorldMatPlug = targetTransFn.findPlug("worldMatrix", &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	MPlug constrainedParWorldMatPlug = constrainedTransFn.findPlug("parentMatrix", &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	MDGModifier dgMod;

	dgMod.connect(targetWorldMatPlug.elementByLogicalIndex(0), depNodeFn.findPlug("targetWorldMatrix"));
	dgMod.connect(constrainedParWorldMatPlug.elementByLogicalIndex(0), depNodeFn.findPlug("constraintParentMatrix"));
	dgMod.connect(timeNode.findPlug("outTime"), depNodeFn.findPlug("inTime"));

	if(! skipX && ! skipY && ! skipZ)
	{
		status = dgMod.connect(depNodeFn.findPlug("constraintTranslate"), constrainedTransFn.findPlug("translate"));
		CHECK_MSTATUS_AND_RETURN_IT(status);
	} else {
		if(! skipX)
		{
			status = dgMod.connect(depNodeFn.findPlug("constraintTranslateX"), constrainedTransFn.findPlug("translateX"));
			CHECK_MSTATUS_AND_RETURN_IT(status);
		}
		if(! skipY)
		{
			status = dgMod.connect(depNodeFn.findPlug("constraintTranslateY"), constrainedTransFn.findPlug("translateY"));
			CHECK_MSTATUS_AND_RETURN_IT(status);
		}
		if(! skipZ)
		{
			status = dgMod.connect(depNodeFn.findPlug("constraintTranslateZ"), constrainedTransFn.findPlug("translateZ"));
			CHECK_MSTATUS_AND_RETURN_IT(status);
		}
	}

	MPlug distancePlug = depNodeFn.findPlug("distance", &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	distancePlug.setDouble(distanceValue);

	MPlug startFramePlug = depNodeFn.findPlug("startFrame");
	startFramePlug.setDouble(startFrame);

	MPlug startPositionXPlug = depNodeFn.findPlug("startPositionX");
	MPlug startPositionYPlug = depNodeFn.findPlug("startPositionY");
	MPlug startPositionZPlug = depNodeFn.findPlug("startPositionZ");
	startPositionXPlug.setDouble(startVector.x);
	startPositionYPlug.setDouble(startVector.y);
	startPositionZPlug.setDouble(startVector.z);

	MPlug lastPositionXPlug = depNodeFn.findPlug("lastPositionX");
	MPlug lastPositionYPlug = depNodeFn.findPlug("lastPositionY");
	MPlug lastPositionZPlug = depNodeFn.findPlug("lastPositionZ");
	lastPositionXPlug.setDouble(startVector.x);
	lastPositionYPlug.setDouble(startVector.y);
	lastPositionZPlug.setDouble(startVector.z);

	MPlug constPositionXPlug = depNodeFn.findPlug("constraintTranslateX");
	MPlug constPositionYPlug = depNodeFn.findPlug("constraintTranslateY");
	MPlug constPositionZPlug = depNodeFn.findPlug("constraintTranslateZ");

	status = dgMod.newPlugValueDouble(constPositionXPlug, startVector.x);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	status = dgMod.newPlugValueDouble(constPositionYPlug, startVector.y);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	status = dgMod.newPlugValueDouble(constPositionZPlug, startVector.z);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	status = dgMod.doIt();
	CHECK_MSTATUS_AND_RETURN_IT(status);

	setResult(nodeName);

	return MStatus::kSuccess;

}

bool sePushPullConstraintCmd::isUndoable() const
{
	return true;
}

MSyntax sePushPullConstraintCmd::newSyntax()
{
	MSyntax syntax;
	// add selection as part of our arguments
	syntax.useSelectionAsDefault(true);
	// make it a selection list type and make sure they give us 2 objects
	syntax.setObjectType(MSyntax::kSelectionList, 2, 2);

	// create the flags (see above code
	syntax.addFlag(nodeNameFlag, nodeNameFlagLong, MSyntax::kString);
	syntax.addFlag(distanceFlag, distanceFlagLong, MSyntax::kDouble);
	syntax.addFlag(startFrameFlag, startFrameFlagLong, MSyntax::kDouble);
	syntax.addFlag(startPositionFlag, startPositionFlagLong, MSyntax::kDouble, MSyntax::kDouble, MSyntax::kDouble);
	syntax.addFlag(skipFlag, skipFlagLong, MSyntax::kString);
	syntax.makeFlagMultiUse(skipFlag);

	// this is not editable or queryable 
	syntax.enableEdit(false);
	syntax.enableQuery(false);

	return syntax;
}