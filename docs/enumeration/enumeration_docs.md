# Enumeration - documentation

## Why is it needed

Some modules can have several instances connected at the same time. These "non-exclusive" modules, have to be given an enumeration between 1-12. 
> non-exclusive modules with an Undefined instance cannot be used

## How to use
A non-exclusive modules instance enumeration can be set by using the onboard button. Pushing the buttom changes the device to Reserve state (fast blinking LED). In this state, the user can cycle through instances 1-12. To select the currently shown instance, don't interact with the button for about 3 seconds. The LED will either light up with the selected instance color (this means the instance was successfully reserved), or start blinking red (this means that this instance is allready reserved and cannot be used). Current / wanted instance is shown by the LED where every instance has it's own color:

![[imgs/instance_colors.png]]

> After reserving successfully, the reserved instance is saved into the modules EEPROM memory and will be attempted to be reserver by the moduled on next boot.
## CAN messages

**reserve(instance_id)** - tells other modules that some of the other modules wants this instance_id.

**collision(instance_id)** - tells other modules that some of the other modules allready has or wants the same instance_id as some other module.

**set(UID,instance_id)** - tells a specific module with that UID, that he should set it's instance_id to the given one (starts the reserve process).

**collision_notify(UID,instance_id)**- reported by a module actively in collision with another module to the rest-API at a given interval.


## Possible scenarios

*following images are generated using PlantUML which has some limitations. Please ignore the dotted line messages.*
***set(instance_id)** message from User should be interpreted as using the physical button on individual modules. It could also be set by set(UID,instance_id) CAN message from API.*
***register(instance_id)** message should be interpreted as successfully finnishing the reserve process*

---

### All modules boot quickly enough so that no module finishes reserve process faster than any other module takes to boot and there are no collisions
- Ideal scenario

![[imgs/successfull_boot.png]]

---

### One module finnishes the reserve process, then another modules conflicts with its instance_id
- Would happen if a new module was connected during runtime or if there is a big difference in boot time for two conflicting modules.

![[imgs/collision_runtime.png]]

---

### Two modules conflict before the reserve process is finnished for both of them
- Would happen after connecting a new module.
![[imgs/collision_boot.png]]

---

## Possible states

**on startup**
- Goes into the reserve state, asking for the last instance_id used (read from the memory). If no record is found, uses the default instance: 1.

**Reserve state**
- The module waits for *collision* message from other modules.
- The module waits for a *reserve*  message from other modules, if the others module instance_id is the same as it's requested id, then it send a *collision* with the requested id.
- After some time, changes to the registered state with the requested id being used as it's instance_id. It also writes the instance_id to onboard memory as last registered.

**Collision state**
- The module periodically sends a *collision_notify* message every *x* milliseconds.
- *collision_notify* is not interpreted in any way by the enumerator component and is only for rest-API.

**Registered state**
- The module holds onto it's id and sends *collision* messages when recieving *reserve* message with it's id.