#ifndef _sePushPullConstraintCmd
#define _sePushPullConstraintCmd

#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>
#include <maya/MStatus.h>
#include <maya/MDGModifier.h>
#include <maya/MString.h>
#include <maya/MVector.h>
#include <maya/MSelectionList.h>

class sePushPullConstraintCmd : public MPxCommand
{
public:
                                 sePushPullConstraintCmd();
        virtual                 ~sePushPullConstraintCmd(); 

        virtual MStatus         doIt(const MArgList& args);
		virtual MStatus			undoIt();
		virtual MStatus			redoIt();
		virtual bool			isUndoable() const;

        static void*    creator();
		static MSyntax	newSyntax();

private:
		MString nodeName;
		bool skipX;
		bool skipY;
		bool skipZ;
		double distanceValue;
		double startFrame;
		MVector startVector;
		MSelectionList sList;
};

#endif