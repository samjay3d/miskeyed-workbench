// Workbench viewport contract.
//
// These fields are ordinary reflected parameters. Users can inspect, edit, or replace
// the camera implementation without relying on undocumented host uniform names.
struct WorkbenchViewportCamera
{
    [UIGroup("Camera")] [UIName("Yaw")]      [UIWidget("angle")]  [UIRange(-3.14159, 3.14159)] [UIStep(0.01)] [UIUnits("rad")]
    float camYaw;
    [UIGroup("Camera")] [UIName("Pitch")]    [UIWidget("angle")]  [UIRange(-1.5, 1.5)]         [UIStep(0.01)] [UIUnits("rad")]
    float camPitch;
    [UIGroup("Camera")] [UIName("Distance")] [UIWidget("slider")] [UIRange(-4.0, 35.0)]        [UIStep(0.05)]
    float camDistance;
    [UIGroup("Camera")] [UIName("FOV")]      [UIWidget("slider")] [UIRange(-40.0, 75.0)]       [UIStep(1.0)] [UIUnits("deg")]
    float camFov;
    [UIGroup("Camera")] [UIName("Pan X")]    [UIWidget("slider")] [UIRange(-10.0, 10.0)]       [UIStep(0.01)]
    float camPanX;
    [UIGroup("Camera")] [UIName("Pan Y")]    [UIWidget("slider")] [UIRange(-10.0, 10.0)]       [UIStep(0.01)]
    float camPanY;
};
uniform WorkbenchViewportCamera workbenchCamera;
