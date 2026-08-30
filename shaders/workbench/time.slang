// Host-managed deterministic evaluation state.
//
// This contract is authored separately so it can become `miskeyed.time` when embedded
// modules are exposed through Slang's filesystem/import seam. Until then the compiler's
// small built-in contract publishes the same declaration automatically.
struct WorkbenchTime
{
    float time;
    float deltaTime;
    uint frame;
    float frameRate;
};
uniform WorkbenchTime workbenchTime;
