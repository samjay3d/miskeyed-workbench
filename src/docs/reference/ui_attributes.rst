UI attribute reference
======================

``UIName(text)``
   Human-readable label.
``UIGroup(text)``
   Inspector group.
``UIWidget(kind)``
   Requested widget such as ``slider`` or ``angle``.
``UIRange(min, max)``
   Bounded numeric range.
``UIStep(value)``
   Numeric increment.
``UITooltip(text)``
   Explanatory hover text.
``UIUnits(text)``
   Semantic display units.

Unsupported presentation hints do not change the reflected GPU byte layout.
