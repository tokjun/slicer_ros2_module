/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

// SlicerRos2 Logic includes
#include "vtkSlicerSlicerRos2Logic.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLTransformStorageNode.h>
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLLinearTransformNode.h>

// VTK includes
#include <vtkIntArray.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkMatrix4x4.h>
#include "vtkCubeSource.h"
#include <vtkTransform.h>

#include <qSlicerCoreIOManager.h>

// STD includes
#include <cassert>

// KDL include_directories
#include "kdl_parser/kdl_parser.hpp"
#include<iostream>
#include <kdl/chain.hpp>
#include <kdl/chainfksolver.hpp>
#include <kdl/chainfksolverpos_recursive.hpp>
#include <kdl/frames_io.hpp>
#include <urdf/model.h>
#include <boost/function.hpp>

#include <stdlib.h>     /* getenv, to be removed later */
#include <boost/shared_ptr.hpp>

// Python includes
#include "vtkSlicerConfigure.h"
#ifdef Slicer_USE_PYTHONQT
#include "PythonQt.h"
#endif

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerSlicerRos2Logic);

//----------------------------------------------------------------------------
vtkSlicerSlicerRos2Logic::vtkSlicerSlicerRos2Logic()
{
}

//----------------------------------------------------------------------------
vtkSlicerSlicerRos2Logic::~vtkSlicerSlicerRos2Logic()
{
}

//----------------------------------------------------------------------------
void vtkSlicerSlicerRos2Logic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSlicerSlicerRos2Logic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerSlicerRos2Logic::RegisterNodes()
{
  assert(this->GetMRMLScene() != 0);
}

//---------------------------------------------------------------------------
void vtkSlicerSlicerRos2Logic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);
}

//---------------------------------------------------------------------------
void vtkSlicerSlicerRos2Logic
::OnMRMLSceneNodeAdded(vtkMRMLNode* vtkNotUsed(node))
{
}

//---------------------------------------------------------------------------
void vtkSlicerSlicerRos2Logic
::OnMRMLSceneNodeRemoved(vtkMRMLNode* vtkNotUsed(node))
{
}

//----------------------------------------------------------------------------
void vtkSlicerSlicerRos2Logic
::loadRobotSTLModels()
{
  char * pHome = getenv ("HOME");
  const std::string home(pHome);


  // Parser the urdf file into an urdf model - to get names of links and pos/ rpy
  urdf::Model my_model;
  if (!my_model.initFile(home + "/ros2_ws/src/SlicerRos2/models/omni.urdf")){
      return;
  }

  // load urdf file into a kdl tree to do forward kinematics
  KDL::Tree my_tree;
  if (!kdl_parser::treeFromUrdfModel(my_model, my_tree)){
    return; //std::cerr << "No urdf file to load." << filename << std::endl;
  }

  // Get the model names from the KDL tree instead of hard coding
  // Get the parent link
  //std::shared_ptr<const urdf::Joint> joint;
  //std::shared_ptr<const urdf::Link> link = my_model.getLink("torso");
  std::shared_ptr<const urdf::Link> root = my_model.getRoot();
  std::string root_name = root->name.c_str();
  std::vector< std::shared_ptr< urdf::Link > > child_links =  root->child_links;
  // get the name of the child link - one the next consecutive one - will need to do this for all of them 
  for (std::shared_ptr< urdf::Link > i: child_links)
    std::cerr << i->name.c_str() << std::endl;
  //std::cerr << root->child_links << std::endl;

  //Call load STL model functions with python - can't find C++ implementation
  #ifdef Slicer_USE_PYTHONQT
    PythonQt::init();
    PythonQtObjectPtr context = PythonQt::self()->getMainModule();
    context.evalScript(QString(
    "import slicer \n"
    "from pathlib import Path \n"
    "mesh_dir = str(Path.home()) + '/ros2_ws/src/SlicerRos2/models/meshes/' \n"
    "slicer.util.loadModel(mesh_dir + 'base.stl') \n"
    "slicer.util.loadModel(mesh_dir + 'torso.stl') \n"
    "slicer.util.loadModel(mesh_dir + 'wrist.stl') \n"
    "slicer.util.loadModel(mesh_dir + 'upper_arm.stl') \n"
    "slicer.util.loadModel(mesh_dir + 'tip.stl') \n"
    "slicer.util.loadModel(mesh_dir + 'stylus.stl') \n"
    "slicer.util.loadModel(mesh_dir + 'lower_arm.stl') \n"));
  #endif

  // Hard coded for now
  const char *link_names[7] = { "base", "torso", "upper_arm", "lower_arm", "wrist", "tip", "stylus"};
  const char *link_names_FK[6] = { "forwardKin_torso", "forwardKin_upperarm", "forwardKin_lower_arm", "forwardKin_wrist", "forwardKin_tip", "forwardKin_stylus"};
  double link_translation_x[7] = {0, 0, 0.0075, 0, 0, 0, 0};
  double link_translation_y[7] = {-0.02, 0, 0, 0, 0, 0, -0.039}; // these two are kinda hacked
  double link_translation_z[7] = { 0, 0.036, 0, 0, 0, 0, 0};
  double link_rotation_x[7] = {0, -90, 0, 90, 180, -90, 90}; // need to find out why angle values are wrong and direction changes
  double link_rotation_y[7] = {0, 0, 0, 0, 0, 0, 90};
  double link_rotation_z[7] = {0, 0, 0, 0, 0, 0, 0};
  // Note the order of trasnforms for each stl model is whats screwing stuff up (/ might need to translate the lower )


  KDL::Chain kdl_chain;
  std::string base_frame("base"); // Specify the base to tip you want ie. joint 1 to 2 (base to torso)
  std::string tip_frame("stylus");
  if (!my_tree.getChain(base_frame, tip_frame, kdl_chain))
  {
    std::cerr << "not working" << std::endl;
    return;
  }
  unsigned int nj = kdl_chain.getNrOfSegments();
  std::cerr << "The chain has this many segments" << std::endl;
  std::cerr << nj << std::endl;

  // Set up an std vector of frames
  std::vector<KDL::Frame> FK_frames;
  FK_frames.resize(nj);
  for (KDL::Frame i: FK_frames)
    std::cout << i << ' ';


  std::cerr << "This is the joint position array" << std::endl;
  auto jointpositions = KDL::JntArray(nj);
  //How to print it
  for (size_t q = 0; q < nj; q++){
    std::cout << jointpositions.operator()(q) << std::endl;
    if (q == 1){
      jointpositions.operator()(q) = 1.0; // Upper arm angle in radians
    }
    std::cout << jointpositions.operator()(q) << std::endl;
  }

  // Initialize the fk solver
  auto fksolver = KDL::ChainFkSolverPos_recursive(kdl_chain);

  // Calculate forward position kinematics
  bool kinematics_status;
  kinematics_status = fksolver.JntToCart(jointpositions,FK_frames);
  if (kinematics_status) {
      std::cout << "Thanks KDL!" <<std::endl;
  } else {
      printf("%s \n","Error: could not calculate forward kinematics :(");
  }

  // Now we have an std vector of KDL frames with the correct kinematics
  std::cout << "After FK Solver" <<std::endl;
  for (KDL::Frame i: FK_frames)
    std::cout << i << ' ';

  // Create a vtkMRMLTransform Node for each of these frames
  for (int l = 0; l < 6; l++){
    vtkNew<vtkMRMLTransformStorageNode> storageNode;
    vtkSmartPointer<vtkMRMLTransformNode> tnode;
    storageNode->SetScene(this->GetMRMLScene());
    vtkNew<vtkMRMLTransformNode> generalTransform;
    generalTransform->SetScene(this->GetMRMLScene());
    tnode = vtkSmartPointer<vtkMRMLTransformNode>::Take(vtkMRMLLinearTransformNode::New());
    storageNode->ReadData(tnode.GetPointer());
    tnode->SetName(link_names_FK[l]);
    this->GetMRMLScene()->AddNode(storageNode.GetPointer());
    this->GetMRMLScene()->AddNode(tnode);
    tnode->SetAndObserveStorageNodeID(storageNode->GetID());

    //Get the matrix and update it based on the forward kinematics
    KDL::Frame cartpos;
    cartpos = FK_frames[l];
    vtkMatrix4x4 *matrix = vtkMatrix4x4::SafeDownCast(tnode->GetMatrixTransformToParent());
    for (int i = 0; i < 4; i++) {
      for (int j=0; j <4; j ++){
        matrix->SetElement(i,j, cartpos.operator()(i,j));
      }
    }
    // Update the matrix for the transform
    tnode->SetMatrixTransformToParent(matrix);

  }

  // Set up the initial position for each link (Rotate and Translate based on origin and rpy from the urdf file)
  for (int k = 0; k < 7; k ++){
    vtkNew<vtkMRMLTransformStorageNode> storageNode;
    vtkSmartPointer<vtkMRMLTransformNode> tnode;
    storageNode->SetScene(this->GetMRMLScene());
    vtkNew<vtkMRMLTransformNode> generalTransform;
    generalTransform->SetScene(this->GetMRMLScene());
    tnode = vtkSmartPointer<vtkMRMLTransformNode>::Take(vtkMRMLLinearTransformNode::New());
    storageNode->ReadData(tnode.GetPointer());
    tnode->SetName("InitialPosition");
    this->GetMRMLScene()->AddNode(storageNode.GetPointer());
    this->GetMRMLScene()->AddNode(tnode);
    tnode->SetAndObserveStorageNodeID(storageNode->GetID());

    vtkTransform *modifiedTransform = vtkTransform::SafeDownCast(tnode->GetTransformToParent());

    //TODO: this needs to be different (FK * (STL_r * STL_tr)) - I think the order is messing stuff up
    modifiedTransform->Translate(link_translation_x[k], link_translation_y[k], link_translation_z[k]);
    tnode->SetAndObserveTransformToParent(modifiedTransform);
    tnode->Modified();
    vtkTransform *modifiedTransform2 = vtkTransform::SafeDownCast(tnode->GetTransformToParent());
    modifiedTransform2->RotateZ(link_rotation_z[k]);
    modifiedTransform2->RotateY(link_rotation_y[k]);
    modifiedTransform2->RotateX(link_rotation_x[k]);
    tnode->SetAndObserveTransformToParent(modifiedTransform2);
    tnode->Modified();

    // Initialize the LPStoRAS transform
    vtkNew<vtkMRMLTransformStorageNode> storageNode3;
    vtkSmartPointer<vtkMRMLTransformNode> LPSToRAS;
    storageNode->SetScene(this->GetMRMLScene());
    vtkNew<vtkMRMLTransformNode> generalTransform3;
    generalTransform3->SetScene(this->GetMRMLScene());
    LPSToRAS = vtkSmartPointer<vtkMRMLTransformNode>::Take(vtkMRMLLinearTransformNode::New());
    storageNode3->ReadData(LPSToRAS.GetPointer());
    LPSToRAS->SetName("LPSToRAS");
    this->GetMRMLScene()->AddNode(storageNode3.GetPointer());
    this->GetMRMLScene()->AddNode(LPSToRAS);
    LPSToRAS->SetAndObserveStorageNodeID(storageNode3->GetID());

    vtkNew<vtkMatrix4x4> LPSToRAS_matrix;
    LPSToRAS_matrix->SetElement(0,0,-1);
    LPSToRAS_matrix->SetElement(1,1,-1);
    LPSToRAS->SetMatrixTransformToParent(LPSToRAS_matrix);
    LPSToRAS->Modified();


    if (k == 0){
      vtkMRMLModelNode *modelNode = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetFirstNodeByName(link_names[k]));
      LPSToRAS->SetAndObserveTransformNodeID(tnode->GetID());
      modelNode->SetAndObserveTransformNodeID(LPSToRAS->GetID());
    }
    else{
      vtkMRMLTransformNode *transformNode = vtkMRMLTransformNode::SafeDownCast(this->GetMRMLScene()->GetFirstNodeByName(link_names_FK[k-1]));
      tnode->SetAndObserveTransformNodeID(transformNode->GetID());

      LPSToRAS->SetAndObserveTransformNodeID(tnode->GetID());

      vtkMRMLModelNode *modelNode = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetFirstNodeByName(link_names[k]));
      modelNode->SetAndObserveTransformNodeID(LPSToRAS->GetID());
    }
  }
}
