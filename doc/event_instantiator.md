EventInstantiator Class

The event entries defined in event_info.json has device patterns in format of "*[n-m]*" or "*[n]*", etc. to keep the json file tidy, clean and avoiding duplications. Those info in the json file will be loaded during eventing_main start (class EventInfo::loadFromFile function handles it). Since it contains the patterns, the device info in it needs to be instantianized based on the giving device_id for next event handlers to process and finally generate the log entry with specific device info.

The class EventInstantiator defined in event_instantiator.cpp/.hpp is used to do that instantiation.
It inherits from class EventHandler.

Input
- event entry info (class EventNode) which contain patterns. variable name: event_node
- device id (std::string): the singlar device name in format of <device>_<index>. variable name: device_id

Process
- Using class DeviceIdPattern consuming device_id and every field in event_node that has patterns to generate a new EventNode object (variable name: event_instnace) that only contains instantianized device info.

Output
- the variable: event_instance

Coding Style
- OOP style
- tidy and clean
- Minimal codes but high performance
