// Workbench reflected-control attributes.
//
// These declarations are prepended before authored source so attributes remain concise.
// Workbench's Headers panel shows this exact source; it is not an undocumented contract.
[__AttributeUsage(_AttributeTargets.Var)] struct UINameAttribute    { string name; }
[__AttributeUsage(_AttributeTargets.Var)] struct UIRangeAttribute   { float min; float max; }
[__AttributeUsage(_AttributeTargets.Var)] struct UIStepAttribute    { float step; }
[__AttributeUsage(_AttributeTargets.Var)] struct UIWidgetAttribute  { string kind; }
[__AttributeUsage(_AttributeTargets.Var)] struct UIGroupAttribute   { string group; }
[__AttributeUsage(_AttributeTargets.Var)] struct UITooltipAttribute { string text; }
[__AttributeUsage(_AttributeTargets.Var)] struct UIUnitsAttribute   { string units; }
