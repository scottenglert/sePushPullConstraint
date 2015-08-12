#    sePushPullConstraint - A constraint plugin for Autodesk's Maya
#    Copyright (C) 2014  Scott Englert - scott@scottenglert.com
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.

import maya.OpenMayaMPx as OpenMayaMPx
import maya.OpenMaya as OpenMaya
import maya.cmds as cmds

class sePushPullConstraintNode(OpenMayaMPx.MPxNode):
    '''sePushPullConstraint node class'''
    kPluginNodeId = OpenMaya.MTypeId(0x0011A640)

    ## DEFINE THE ATTRIBUTES - PLACE HOLDERS
    # constraint output attribute
    constTransAttr = OpenMaya.MObject() 
    ctAttrX = OpenMaya.MObject()
    ctAttrY = OpenMaya.MObject()
    ctAttrZ = OpenMaya.MObject()
    
    # last position attribute
    lastPositionAttr = OpenMaya.MObject()
    lpAttrX = OpenMaya.MObject()
    lpAttrY = OpenMaya.MObject()
    lpAttrZ = OpenMaya.MObject()
    
    # start position attribute
    startPositionAttr = OpenMaya.MObject()
    spAttrX = OpenMaya.MObject()
    spAttrY = OpenMaya.MObject()
    spAttrZ = OpenMaya.MObject()
    
    # target and constraint matrix attributes
    targetAttr = OpenMaya.MObject()
    constraintParentAttr = OpenMaya.MObject()
    
    # others
    distanceAttr = OpenMaya.MObject()
    inTimeAttr = OpenMaya.MObject()
    startFrameAttr = OpenMaya.MObject()
    pullAttr = OpenMaya.MObject()
    pushAttr = OpenMaya.MObject()
    
    def __init__(self):
        OpenMayaMPx.MPxNode.__init__(self)

    def compute(self, plug, data):
        '''Does all the computing when the node needs to evaluate'''
        
        # get the root plug if this is a child plug
        if plug.isChild():
            plugRoot = plug.parent()
        else:
            plugRoot = plug
        
        # check the plug is the output one we need to compute for
        if plugRoot != sePushPullConstraintNode.constTransAttr:
            return OpenMaya.MStatus.kUnknownParameter
        
        # get the current and start frame
        currentFrame = data.inputValue(sePushPullConstraintNode.inTimeAttr).asTime()
        startFrame = data.inputValue(sePushPullConstraintNode.startFrameAttr).asDouble()
    
        # get the output attributes
        outputHandle = data.outputValue(sePushPullConstraintNode.constTransAttr)
    
        # if the current frame is before the start frame set the current position
        # as the start position and return as completed
        if currentFrame.value() < startFrame:
            startPos = data.inputValue(sePushPullConstraintNode.startPositionAttr).asVector()
            lastPosHandle = data.outputValue(sePushPullConstraintNode.lastPositionAttr)
            
            lastPosHandle.setMVector(startPos)
            outputHandle.setMVector(startPos)
            
            data.setClean(sePushPullConstraintNode.constTransAttr)
            data.setClean(sePushPullConstraintNode.lastPositionAttr)
            
            return
        
        # check if either the push or pull toggles are on to save a little time
        isPullActive = data.inputValue(sePushPullConstraintNode.pullAttr).asBool()
        isPushActive = data.inputValue(sePushPullConstraintNode.pushAttr).asBool()
        
        if not isPullActive and not isPushActive:
            # no need to go any further since nothing will change
            data.setClean(sePushPullConstraintNode.constTransAttr)
            return
        
        # use the last position we stored on the previous calculation
        lp = data.inputValue(sePushPullConstraintNode.lastPositionAttr).asVector()
        lastPosition = OpenMaya.MPoint(lp)

        # get the target world matrix and store the translation
        targetMat = data.inputValue(sePushPullConstraintNode.targetAttr).asMatrix()
        targetPos = OpenMaya.MPoint(targetMat(3,0), targetMat(3,1), targetMat(3,2))
                
        # get the distance
        dist = data.inputValue(sePushPullConstraintNode.distanceAttr).asDouble()
        
        # get the constrained object parent matrix attribute
        constraintParentMat = data.inputValue(sePushPullConstraintNode.constraintParentAttr).asMatrix()

        # we get the relative vector from the target to the constrained
        relativePos = OpenMaya.MVector((lastPosition * constraintParentMat) - targetPos)
        
        # get distance between these two objects
        currentDistance = relativePos.length()

        # if the pull is on and the current distance is greater than our set distance
        if (isPullActive and (currentDistance > dist)) or (isPushActive and (currentDistance < dist)):
            # normalize the local vector to unit length of 1
            relativePos.normalize()
            relativePos *= dist
            # add the new local position to the target giving us were need to place the constraint object
            newPosition = targetPos + relativePos
            # get the inverse of the parent matrix 
            localPos = OpenMaya.MVector(newPosition * constraintParentMat.inverse())
            # set the value to the plugs
            outputHandle.setMVector(localPos)
            lastPosOutHandle = data.outputValue(sePushPullConstraintNode.lastPositionAttr)
            lastPosOutHandle.setMVector(localPos)
            
            data.setClean(sePushPullConstraintNode.lastPositionAttr)

        # set the plug to clean
        data.setClean(sePushPullConstraintNode.constTransAttr)
        
def nodeCreator():
    '''Creates and returns a new instance of the node'''
    return OpenMayaMPx.asMPxPtr(sePushPullConstraintNode())

def nodeInitialize():
    '''Handles adding the attributes to the node'''
    numericAttr = OpenMaya.MFnNumericAttribute()
    unitAttr = OpenMaya.MFnUnitAttribute()
    matrixAttr = OpenMaya.MFnMatrixAttribute()

    # create the output translation attribute
    sePushPullConstraintNode.ctAttrX = numericAttr.create("constraintTranslateX", "ctx", OpenMaya.MFnNumericData.kDouble, 0.0)
    sePushPullConstraintNode.ctAttrY = numericAttr.create("constraintTranslateY", "cty", OpenMaya.MFnNumericData.kDouble, 0.0)
    sePushPullConstraintNode.ctAttrZ = numericAttr.create("constraintTranslateZ", "ctz", OpenMaya.MFnNumericData.kDouble, 0.0)
    sePushPullConstraintNode.constTransAttr = numericAttr.create("constraintTranslate", "ct",
                                                                 sePushPullConstraintNode.ctAttrX,
                                                                 sePushPullConstraintNode.ctAttrY,
                                                                 sePushPullConstraintNode.ctAttrZ)
    numericAttr.setWritable(False)
    sePushPullConstraintNode.addAttribute(sePushPullConstraintNode.constTransAttr)

    # create the in time attribute
    sePushPullConstraintNode.inTimeAttr = unitAttr.create("inTime", "it",  OpenMaya.MFnUnitAttribute.kTime, 1.0)
    unitAttr.setStorable(False)
    unitAttr.setKeyable(False)
    unitAttr.setHidden(True)
    sePushPullConstraintNode.addAttribute(sePushPullConstraintNode.inTimeAttr)
    sePushPullConstraintNode.attributeAffects(sePushPullConstraintNode.inTimeAttr, sePushPullConstraintNode.constTransAttr)
    
    # create the start frame attribute
    sePushPullConstraintNode.startFrameAttr = numericAttr.create("startFrame", "stf", OpenMaya.MFnNumericData.kDouble, 1.0)
    numericAttr.setKeyable(True)
    sePushPullConstraintNode.addAttribute(sePushPullConstraintNode.startFrameAttr)
    sePushPullConstraintNode.attributeAffects(sePushPullConstraintNode.startFrameAttr, sePushPullConstraintNode.constTransAttr)
    
    # create the distance attribute
    sePushPullConstraintNode.distanceAttr = numericAttr.create("distance", "dist", OpenMaya.MFnNumericData.kDouble)
    numericAttr.setKeyable(True)
    numericAttr.setMin(0.0)
    sePushPullConstraintNode.addAttribute(sePushPullConstraintNode.distanceAttr)
    sePushPullConstraintNode.attributeAffects(sePushPullConstraintNode.distanceAttr, sePushPullConstraintNode.constTransAttr)
    
    # create the target world matrix attribute
    sePushPullConstraintNode.targetAttr = matrixAttr.create("targetWorldMatrix", "twm", OpenMaya.MFnMatrixAttribute.kDouble)
    matrixAttr.setStorable(False)
    sePushPullConstraintNode.addAttribute(sePushPullConstraintNode.targetAttr)
    sePushPullConstraintNode.attributeAffects(sePushPullConstraintNode.targetAttr, sePushPullConstraintNode.constTransAttr)

    # create the constraint parent matrix attribute    
    sePushPullConstraintNode.constraintParentAttr = matrixAttr.create("constraintParentMatrix", "cpm", OpenMaya.MFnMatrixAttribute.kDouble)
    matrixAttr.setStorable(False)
    sePushPullConstraintNode.addAttribute(sePushPullConstraintNode.constraintParentAttr)
    sePushPullConstraintNode.attributeAffects(sePushPullConstraintNode.constraintParentAttr, sePushPullConstraintNode.constTransAttr)
    
    # create the start position attribute
    sePushPullConstraintNode.spAttrX = numericAttr.create("startPositionX", "spx", OpenMaya.MFnNumericData.kDouble, 0.0)
    sePushPullConstraintNode.spAttrY = numericAttr.create("startPositionY", "spy", OpenMaya.MFnNumericData.kDouble, 0.0)
    sePushPullConstraintNode.spAttrZ = numericAttr.create("startPositionZ", "spz", OpenMaya.MFnNumericData.kDouble, 0.0)
    sePushPullConstraintNode.startPositionAttr = numericAttr.create("startPosition", "sp",
                                                                    sePushPullConstraintNode.spAttrX,
                                                                    sePushPullConstraintNode.spAttrY,
                                                                    sePushPullConstraintNode.spAttrZ)
    numericAttr.setKeyable(True)
    sePushPullConstraintNode.addAttribute(sePushPullConstraintNode.startPositionAttr)
    sePushPullConstraintNode.attributeAffects(sePushPullConstraintNode.startPositionAttr, sePushPullConstraintNode.constTransAttr)

    # create the push bool attribute
    sePushPullConstraintNode.pushAttr = numericAttr.create("push", "psh", OpenMaya.MFnNumericData.kBoolean, 1.0)
    numericAttr.setKeyable(True)
    sePushPullConstraintNode.addAttribute(sePushPullConstraintNode.pushAttr)
    sePushPullConstraintNode.attributeAffects(sePushPullConstraintNode.pushAttr, sePushPullConstraintNode.constTransAttr)
    
    # create the push bool attribute
    sePushPullConstraintNode.pullAttr = numericAttr.create("pull", "pll", OpenMaya.MFnNumericData.kBoolean, 1.0)
    numericAttr.setKeyable(True)
    sePushPullConstraintNode.addAttribute(sePushPullConstraintNode.pullAttr)
    sePushPullConstraintNode.attributeAffects(sePushPullConstraintNode.pullAttr, sePushPullConstraintNode.constTransAttr)

    # create the last position attribute for internal uses
    sePushPullConstraintNode.lpAttrX = numericAttr.create("lastPositionX", "lpx", OpenMaya.MFnNumericData.kDouble, 0.0)
    sePushPullConstraintNode.lpAttrY = numericAttr.create("lastPositionY", "lpy", OpenMaya.MFnNumericData.kDouble, 0.0)
    sePushPullConstraintNode.lpAttrZ = numericAttr.create("lastPositionZ", "lpz", OpenMaya.MFnNumericData.kDouble, 0.0)
    sePushPullConstraintNode.lastPositionAttr = numericAttr.create("lastPosition", "lp",
                                                                   sePushPullConstraintNode.lpAttrX,
                                                                   sePushPullConstraintNode.lpAttrY,
                                                                   sePushPullConstraintNode.lpAttrZ)
    numericAttr.setHidden(True);
    sePushPullConstraintNode.addAttribute(sePushPullConstraintNode.lastPositionAttr);

def cmdCreator():
    '''Creates and returns and instance of the sePushPullConstraint command'''
    return OpenMayaMPx.asMPxPtr(sePushPullConstraintCmd())

def cmdSyntax():
    '''Creates the syntax for the command'''
    syntax = OpenMaya.MSyntax()
    
    syntax.setObjectType(OpenMaya.MSyntax.kStringObjects)
    syntax.setMaxObjects(2)
    
    syntax.enableEdit(False)
    syntax.enableQuery(False)
    
    syntax.addFlag("-n", "-name", OpenMaya.MSyntax.kString)
    syntax.addFlag("-d", "-distance", OpenMaya.MSyntax.kDouble)
    syntax.addFlag("-sf", "-startFrame", OpenMaya.MSyntax.kDouble)
    syntax.addFlag("-sp", "-startPosition", OpenMaya.MSyntax.kDouble, OpenMaya.MSyntax.kDouble, OpenMaya.MSyntax.kDouble)
    syntax.addFlag("-sk", "-skip", OpenMaya.MSyntax.kString)
    syntax.makeFlagMultiUse("-sk")
    
    return syntax

class sePushPullConstraintCmd(OpenMayaMPx.MPxCommand):
    '''Class that contains all functions related to the sePushPullConstraint command'''
        
    def __init__(self):
        OpenMayaMPx.MPxCommand.__init__(self)
        self.dgMod = OpenMaya.MDGModifier()
        
        self.nodeName = ''
        self.skipX = False
        self.skipY = False
        self.skipZ = False
        self.distanceValue = 0.0
        self.startFrame = 0.0
        self.startVector = OpenMaya.MVector()
        self.sList = OpenMaya.MSelectionList()        
                
    def doIt(self, args):
        '''Creates the node and connects everything based on the parameters given'''
        # get the arguments passed in
        argData = OpenMaya.MArgParser(self.syntax(), args)
        
        # get objects to use in the constraint and make sure there is only two
        objects = []
        argData.getObjects(objects)
        if not objects:
            # use the selection
            objects = cmds.ls(sl=True)
            
        for obj in objects:
            self.sList.add(obj)
        
        # if there is less than two objects given, return an error
        if self.sList.length() < 2:
            raise RuntimeError('Two transforms are required to create constraint.')
        
        # get the target object path and make sure it's a transform type
        targetDAG = OpenMaya.MDagPath()
        try:
            self.sList.getDagPath(0, targetDAG)
        except RuntimeError:
            raise RuntimeError('Target object must be a DAG object type. Unable to get path to object.')

        targetTransFn = OpenMaya.MFnTransform()
        try:
            targetTransFn.setObject(targetDAG)
        except RuntimeError:
            raise RuntimeError('Target object type invalid. You must choose a transform.')
        
        # get the constraint object path and make sure it's a transform type
        constraintDAG = OpenMaya.MDagPath()
        try:
            self.sList.getDagPath(1, constraintDAG)
        except RuntimeError:
            raise RuntimeError('Constraint object must be a DAG object type. Unable to get path to object.')

        constrainedTransFn = OpenMaya.MFnTransform()
        try:
            constrainedTransFn.setObject(constraintDAG)
        except RuntimeError:
            raise RuntimeError('Constraint object type invalid. You must choose a transform.')
                
        numSkips = argData.numberOfFlagUses('-sk')
        if numSkips > 3:
            raise RuntimeError('You can not have more than 3 skip flags.')
        
        if argData.isFlagSet('-n'):
            self.nodeName = argData.flagArgumentString('-n', 0)
        
        for i in range(numSkips):
            argList = OpenMaya.MArgList()
            argData.getFlagArgumentList('-sk', i, argList)
            
            axis = argList.asString(0)

            if axis == 'x':
                self.skipX = True
            elif axis == 'y':
                self.skipY = True
            elif axis == 'z':
                self.skipZ = True
        
        # distance flag
        if argData.isFlagSet('-d'):
            self.distanceValue = argData.flagArgumentDouble('-d', 0)
        else:
            # calculate the distance
            targetPos = targetTransFn.getTranslation(OpenMaya.MSpace.kWorld)
            constrainedPos = constrainedTransFn.getTranslation(OpenMaya.MSpace.kWorld)
            localPos = targetPos - constrainedPos
            self.distanceValue = localPos.length()
            
        # start frame flag
        if argData.isFlagSet('-sf'):
            self.startFrame = argData.flagArgumentDouble('-sf', 0)
        else:
            self.startFrame = cmds.currentTime(q=True)
        
        # start position
        if argData.isFlagSet('-sp'):
            spX = argData.flagArgumentDouble('-sp', 0)
            spY = argData.flagArgumentDouble('-sp', 1)
            spZ = argData.flagArgumentDouble('-sp', 2)
            
            self.startVector = OpenMaya.MVector(spX, spY, spZ)
        else:
            self.startVector = constrainedTransFn.getTranslation(OpenMaya.MSpace.kTransform)
            
        self.redoIt()
        
    def redoIt(self):
        
        targetDAG = OpenMaya.MDagPath()
        self.sList.getDagPath(0, targetDAG)
        targetTransFn = OpenMaya.MFnTransform()
        targetTransFn.setObject(targetDAG)
        
        constrainedDAG = OpenMaya.MDagPath()
        self.sList.getDagPath(1, constrainedDAG)
        constrainedTransFn = OpenMaya.MFnTransform()
        constrainedTransFn.setObject(constrainedDAG)
        
         # get a time node, there should be a better way but I'm not sure how
        dgTimeNodes = OpenMaya.MItDependencyNodes(OpenMaya.MFn.kTime)
        while not dgTimeNodes.isDone():
            # gives us time MObject
            timeObject = dgTimeNodes.thisNode()
            break
        
        timeNode = OpenMaya.MFnDependencyNode(timeObject)
        
        # create the node and rename it
        depNodeFn = OpenMaya.MFnDependencyNode()
        if self.nodeName:
            depNodeFn.create('sePushPullConstraint', self.nodeName)
        else:
            depNodeFn.create('sePushPullConstraint')
        
        # just in case a number was added
        self.nodeName = depNodeFn.name()
            
        # get the plugs to make the connections
        targetWorldMatPlug = targetTransFn.findPlug('worldMatrix')
        constrainedParWorldMatPlug = constrainedTransFn.findPlug('parentMatrix')
        
        dgMod = OpenMaya.MDGModifier()
        
        dgMod.connect(targetWorldMatPlug.elementByLogicalIndex(0), depNodeFn.findPlug('targetWorldMatrix'))
        dgMod.connect(constrainedParWorldMatPlug.elementByLogicalIndex(0), depNodeFn.findPlug('constraintParentMatrix'))
        dgMod.connect(timeNode.findPlug('outTime'), depNodeFn.findPlug('inTime'))
        
        # connecting the translation of constrained transform
        if not self.skipX and not self.skipY and not self.skipZ:
            dgMod.connect(depNodeFn.findPlug('constraintTranslate'), constrainedTransFn.findPlug('translate'))
        else:
            if not self.skipX:
                dgMod.connect(depNodeFn.findPlug('constraintTranslateX'), constrainedTransFn.findPlug('translateX'))
                
            if not self.skipY:
                dgMod.connect(depNodeFn.findPlug('constraintTranslateY'), constrainedTransFn.findPlug('translateY'))
                
            if not self.skipZ:
                dgMod.connect(depNodeFn.findPlug('constraintTranslateZ'), constrainedTransFn.findPlug('translateZ'))
    
        # distance
        distancePlug = depNodeFn.findPlug('distance')
        distancePlug.setDouble(self.distanceValue)
        
        # start frame
        startFramePlug = depNodeFn.findPlug('startFrame')
        startFramePlug.setDouble(self.startFrame)
                
        # start position
        startPositionXPlug = depNodeFn.findPlug('startPositionX')
        startPositionYPlug = depNodeFn.findPlug('startPositionY')
        startPositionZPlug = depNodeFn.findPlug('startPositionZ')
        startPositionXPlug.setDouble(self.startVector.x)
        startPositionYPlug.setDouble(self.startVector.y)
        startPositionZPlug.setDouble(self.startVector.z)
        
        # set the last position
        lastPositionXPlug = depNodeFn.findPlug('lastPositionX')
        lastPositionYPlug = depNodeFn.findPlug('lastPositionY')
        lastPositionZPlug = depNodeFn.findPlug('lastPositionZ')
        lastPositionXPlug.setDouble(self.startVector.x)
        lastPositionYPlug.setDouble(self.startVector.y)
        lastPositionZPlug.setDouble(self.startVector.z)
        
        # set the constraint translate out put so it starts at the right place
        constPositionXPlug = depNodeFn.findPlug('constraintTranslateX')
        constPositionYPlug = depNodeFn.findPlug('constraintTranslateY')
        constPositionZPlug = depNodeFn.findPlug('constraintTranslateZ')
        constPositionXPlug.setDouble(self.startVector.x)
        constPositionYPlug.setDouble(self.startVector.y)
        constPositionZPlug.setDouble(self.startVector.z)

        dgMod.doIt()
        
        self.setResult(self.nodeName)
            
    def undoIt(self):
        cmds.delete(self.nodeName)
    
    def isUndoable(self):
        return True
    
def initializePlugin(obj):
    '''Called when loading the plugin'''
    plugin = OpenMayaMPx.MFnPlugin(obj, 'Scott Englert', '1.2', 'Any')
    try:
        plugin.registerNode('sePushPullConstraint', sePushPullConstraintNode.kPluginNodeId, nodeCreator, nodeInitialize)
        print "sePushPullConstraint  Copyright (C) 2014  Scott Englert - scott@scottenglert.com"
        print "This program comes with ABSOLUTELY NO WARRANTY; for details read the license file."
        print "This is free software, and you are welcome to redistribute it under certain conditions; See license file for details."
    except:
        raise RuntimeError('Failed to register node')
    
    try:
        plugin.registerCommand('sePushPullConstraint', cmdCreator, cmdSyntax)
    except:
        raise RuntimeError('Failed to register command')

def uninitializePlugin(obj):
    '''Called by Maya to unload the plugin'''
    plugin = OpenMayaMPx.MFnPlugin(obj)
    try:
        plugin.deregisterNode(sePushPullConstraintNode.kPluginNodeId)
    except:
        raise RuntimeError('Failed to deregister node')
    
    try:
        plugin.deregisterCommand('sePushPullConstraint')
    except:
        raise RuntimeError('Failed to deregister command')