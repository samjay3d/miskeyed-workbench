// Workbench reflected-control attributes.
//
// Authored shaders import this packaged module so attributes remain concise.
// This authored module source is packaged with Workbench rather than reconstructed in UI code.
[__AttributeUsage(_AttributeTargets.Var)] struct UINameAttribute    { string name; }
[__AttributeUsage(_AttributeTargets.Var)] struct UIRangeAttribute   { float min; float max; }
[__AttributeUsage(_AttributeTargets.Var)] struct UIStepAttribute    { float step; }
[__AttributeUsage(_AttributeTargets.Var)] struct UIWidgetAttribute  { string kind; }
[__AttributeUsage(_AttributeTargets.Var)] struct UIGroupAttribute   { string group; }
[__AttributeUsage(_AttributeTargets.Var)] struct UITooltipAttribute { string text; }
[__AttributeUsage(_AttributeTargets.Var)] struct UIUnitsAttribute   { string units; }
