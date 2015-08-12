/*
sePushPullConstraint is a Maya plugin that will push or pull one object
a specific distance by another.

Copyright (C) 2013  Scott Englert - scott@scottenglert.com

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "sePushPullConstraintNode.h"
#include "sePushPullConstraintCmd.h"
#include <maya/MFnPlugin.h>

MTypeId sePushPullConstraint::id(0x0011A640);

MStatus initializePlugin(MObject obj)
{ 
	MStatus status;
	MFnPlugin plugin(obj, "Scott Englert", "1.2", "Any");
	
	status = plugin.registerNode("sePushPullConstraint", sePushPullConstraint::id, sePushPullConstraint::creator, sePushPullConstraint::initialize);
	if (!status) {
		status.perror("registerNode");
		return status;
	}

	status = plugin.registerCommand("sePushPullConstraint", sePushPullConstraintCmd::creator, sePushPullConstraintCmd::newSyntax);
    if (!status) {
            status.perror("registerCommand");
            return status;
    }

    return status;
}

MStatus uninitializePlugin(MObject obj)
{
	MStatus status;
	MFnPlugin plugin(obj);

	status = plugin.deregisterNode(sePushPullConstraint::id);
	if(!status){
		status.perror("deregisterNode");
		return status;
	}
	
	status = plugin.deregisterCommand("sePushPullConstraint");
    if(!status){
		status.perror("deregisterCommand");
        return status;
    }

	return status;
}