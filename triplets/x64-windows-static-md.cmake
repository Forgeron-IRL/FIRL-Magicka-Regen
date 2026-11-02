# Overlay triplet: overrides vcpkg's built-in x64-windows-static-md.
#
# Pins the MSVC v143 toolset (VS 2022, 14.4x) for ALL dependencies built by
# vcpkg. Otherwise vcpkg uses the newest toolset it finds (e.g. a VS preview),
# and the static libs end up referencing newer STL symbols missing from the
# v143 msvcprt.lib → LNK2001. With v143 everywhere, the DLL only needs the
# VC++ 2015-2022 redistributable that players already have.
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_PLATFORM_TOOLSET v143)
