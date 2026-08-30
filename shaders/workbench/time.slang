// Host-managed deterministic evaluation state.
//
// Packaged as `miskeyed.time` and resolved through Slang's normal import machinery.
struct WorkbenchTime
{
    float time;
    float deltaTime;
    uint frame;
    float frameRate;
};
uniform WorkbenchTime workbenchTime;
