Runtime backend reference
=========================

``Auto``
   D3D11 on Windows, Vulkan on Linux, Metal on macOS.
``D3D11``
   Shipped Windows default; HLSL SM5 through FXC.
``Vulkan``
   Selectable on Windows and default policy on Linux.
``Metal``
   Default policy on macOS.

Backend availability is not itself evidence that a wheel rendered. See
:doc:`portability` for the package, install, contract, and runtime validation matrix.
