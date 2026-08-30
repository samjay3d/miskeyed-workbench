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

Evidence levels differ: wheels build/import on supported platforms; Windows D3D11
and Vulkan additionally have native draw-recording smoke coverage.
