#ifndef __vtkMRMLROS2Tf2BufferLookupNode_h
#define __vtkMRMLROS2Tf2BufferLookupNode_h

// MRML includes
#include <vtkMRMLNode.h>

#include <vtkSlicerROS2ModuleMRMLExport.h>

// forward declaration for internals
class vtkMRMLROS2Tf2BufferLookupInternals;
class vtkMRMLTransformNode;
class vtkMatrix4x4;
class vtkObject;

class VTK_SLICER_ROS2_MODULE_MRML_EXPORT vtkMRMLROS2Tf2BufferLookupNode: public vtkMRMLNode
{

  // friend declarations
  friend class vtkMRMLROS2Tf2BufferLookupInternals;

 public:

  typedef vtkMRMLROS2Tf2BufferLookupNode SelfType;
  vtkTypeMacro(vtkMRMLROS2Tf2BufferLookupNode, vtkMRMLNode);
  static SelfType * New(void);
  vtkMRMLNode * CreateNodeInstance(void) override;
  const char * GetNodeTagName(void) override;
  void PrintSelf(std::ostream& os, vtkIndent indent) override;
  
  bool AddToROS2Node(const char * nodeId);

  void SetParentID(const std::string & parent_id);
  std::string GetParentID();

  void SetChildID(const std::string & child_id);
  std::string GetChildID();

  bool CheckIfParentAndChildSet();

  // Save and load
  virtual void ReadXMLAttributes(const char** atts) override;
  virtual void WriteXML(std::ostream& of, int indent) override;

 protected:
  vtkMRMLROS2Tf2BufferLookupNode();
  ~vtkMRMLROS2Tf2BufferLookupNode();
  
  void UpdateMRMLNodeName();

  std::unique_ptr<vtkMRMLROS2Tf2BufferLookupInternals> mInternals;
  std::string mMRMLNodeName = "ros2:tf2bufferlookup:empty";
  std::string mParentID = "";
  std::string mChildID = "";
  size_t mNumberOfBroadcasts = 0;

};

#endif // _vtkMRMLROS2Tf2BufferLookupNode_h