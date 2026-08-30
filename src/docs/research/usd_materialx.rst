USD and MaterialX direction
===========================

USD is a possible authored scene-meaning layer and MaterialX a possible portable
material-description edge. Neither is part of the shipped Shader Toy or Render Toy
workflow in 0.3.0.

Any future integration should let USD own authored scene meaning and use established
change propagation rather than adding a parallel Workbench scene database.
``ShaderDocument`` remains an independent shader-authoring product unless an explicit
consumer edge is designed and tested.
