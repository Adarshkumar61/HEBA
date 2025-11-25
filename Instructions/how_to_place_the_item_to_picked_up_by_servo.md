How does it know where to place the item?

Same idea:

You decide a “delivery spot” (e.g., near bed edge or on a small tray).

You tune armWaterDeliverPose() / armMedicinePose() so the gripper reaches that spot.

The robot always places/serves in that pre-planned position.

No camera, no detection. Just servo angles.

What about the base movement? How does it know distance?

Again: it doesn’t measure the whole world, it just does timed motion.

In doWaterRoutine():

driveFor(motorsForward, 800);   // go forward for ~0.8s
...
driveFor(motorsBackward, 800);  // go back for ~0.8s


YOU tune that 800 (milliseconds) so robot moves from “home” to “table” and back.

Maybe you even put physical stops (like a wall or block) so it stops at the same point.

Poses:
armHomePose(), armWaterDeliverPose(), armMedicinePose()