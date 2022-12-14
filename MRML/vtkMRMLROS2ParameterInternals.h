#ifndef __vtkMRMLROS2ParameterInternals_h
#define __vtkMRMLROS2ParameterInternals_h

// ROS2 includes
#include <rclcpp/rclcpp.hpp>

#include <vtkMRMLScene.h>
#include <vtkMRMLROS2NODENode.h>
#include <vtkMRMLROS2NodeInternals.h>

class vtkMRMLROS2ParameterInternals
{
public:
  vtkMRMLROS2ParameterInternals(vtkMRMLROS2ParameterNode * mrmlNode):
    mMRMLNode(mrmlNode)
  {}
  virtual ~vtkMRMLROS2ParameterInternals() = default;

  virtual bool AddToROS2Node(vtkMRMLScene * scene, const char * nodeId,
			     const std::string & trackedNodeName, std::string & errorMessage) = 0;
  virtual bool IsAddedToROS2Node(void) const = 0;
  virtual const char * GetROSType(void) const = 0;
  virtual const char * GetSlicerType(void) const = 0;
  virtual std::string GetLastMessageYAML(void) const = 0;

protected:
  vtkMRMLROS2ParameterNode * mMRMLNode;
};


template <typename _ros_type, typename _slicer_type>
class vtkMRMLROS2ParameterTemplatedInternals: public vtkMRMLROS2ParameterInternals
{
public:
  typedef vtkMRMLROS2ParameterTemplatedInternals<_ros_type, _slicer_type> SelfType;

  vtkMRMLROS2ParameterTemplatedInternals(vtkMRMLROS2ParameterNode *  mrmlNode):
    vtkMRMLROS2ParameterInternals(mrmlNode)
  {}

protected:
  _ros_type mLastMessageROS;
  std::shared_ptr<rclcpp::Subscription<_ros_type>> mSubscription = nullptr;
  std::shared_ptr<rclcpp::ParameterEventHandler> param_subscriber_ = nullptr;
  std::shared_ptr<rclcpp::ParameterEventCallbackHandle> cb_handle;

  /**
   * This is the ROS callback for the subscription.  This methods
   * saves the ROS message as-is and set the modified flag for the
   * MRML node
   */
  void ParameterCallback(const _ros_type & message) {
    // \todo is there a timestamp in MRML nodes we can update from the ROS message?
    mLastMessageROS = message;
    mMRMLNode->mNumberOfMessages++;
    mMRMLNode->Modified();
  }

  /**
   * Add the subscriber to the ROS2 node.  This methods searched the
   * vtkMRMLROS2NODENode by Id to locate the rclcpp::node
   */
  bool AddToROS2Node(vtkMRMLScene * scene, const char * nodeId,
		     const std::string & trackedNodeName, std::string & errorMessage) {

    vtkMRMLNode * rosNodeBasePtr = scene->GetNodeByID(nodeId);

    if (!rosNodeBasePtr) {
      errorMessage = "unable to locate node";
      return false;
    }

    vtkMRMLROS2NODENode * rosNodePtr = dynamic_cast<vtkMRMLROS2NODENode *>(rosNodeBasePtr);

    if (!rosNodePtr) {
      errorMessage = std::string(rosNodeBasePtr->GetName()) + " doesn't seem to be a vtkMRMLROS2NODENode";
      return false;
    }

    std::shared_ptr<rclcpp::Node> nodePointer = rosNodePtr->mInternals->mNodePointer;

    param_subscriber_ = std::make_shared<rclcpp::ParameterEventHandler>(nodePointer);

    auto cb = [trackedNodeName, nodePointer](const rcl_interfaces::msg::ParameterEvent &event)
    {
        // Obtain list of parameters that changed
        std::cerr << "Event node :"<< event.node.c_str() << std::endl;
        if(!strcmp(event.node.c_str(),trackedNodeName.c_str())){
          auto params = rclcpp::ParameterEventHandler::get_parameters_from_event(event);

          // Iterate through every parameter in the list
          for (auto &p : params)
          {
              // Create a string message from the info received.
              std::string msgString = "Param Name :" + p.get_name() + " | Param Type : " + p.get_type_name() + " | Param Value : " + p.value_to_string();
              std::cerr << "Received an update to a parameter"  << std::endl;
              std::cerr << msgString << "\n" <<std::endl;
          }
        }
    };

    cb_handle = param_subscriber_->add_parameter_event_callback(cb);

    // mSubscription
    //   = nodePointer->create_subscription<_ros_type>(topic, 100,
		// 				    std::bind(&SelfType::ParameterCallback, this, std::placeholders::_1));

    rosNodePtr->SetNthNodeReferenceID("parameter",
				      rosNodePtr->GetNumberOfNodeReferences("parameter"),
				      mMRMLNode->GetID());
              
    mMRMLNode->SetNodeReferenceID("node", nodeId);

    return true;

  }

  bool IsAddedToROS2Node(void) const override
  {
    return (mSubscription != nullptr);
  }

  const char * GetROSType(void) const override
  {
    return rosidl_generator_traits::name<_ros_type>();
  }

  const char * GetSlicerType(void) const override
  {
    return typeid(_slicer_type).name();
  }

  std::string GetLastMessageYAML(void) const override
  {
    std::stringstream out;
    rosidl_generator_traits::to_yaml(mLastMessageROS, out);
    return out.str();
  }
};

#endif
// __vtkMRMLROS2ParameterInternals_h

