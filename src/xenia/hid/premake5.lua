project_root = "../../.."
include(project_root.."/tools/build")

group("src")
project("xenia-hid")
  uuid("88a4ef38-c550-430f-8c22-8ded4e4ef601")
  kind("StaticLib")
  language("C++")
  links({
    "xenia-base",
  })
  defines({
  })
  includedirs({
    project_root.."/third_party/gflags/src",
  })
  local_platform_files()
  removefiles({"*_demo.cc"})

group("demos")
project("xenia-hid-demo")
  uuid("a56a209c-16d5-4913-85f9-86976fe7fddf")
  kind("WindowedApp")
  language("C++")
  links({
    "gflags",
    "glew",
    "imgui",
    "xenia-base",
    "xenia-hid",
    "xenia-hid-nop",
    "xenia-ui",
  })
  filter("platforms:Linux")
    links({
      "X11",
      "xcb",
      "X11-xcb",
      "GL",
      "vulkan",
    })
  filter()

  flags({
    "WinMain",  -- Use WinMain instead of main.
  })
  defines({
    "GLEW_STATIC=1",
    "GLEW_MX=1",
  })
  includedirs({
    project_root.."/third_party/gflags/src",
  })
  files({
    "hid_demo.cc",
    project_root.."/src/xenia/base/main_"..platform_suffix..".cc",
  })
  files({
  })
  resincludedirs({
    project_root,
  })

  filter("platforms:Windows")
    links({
      "xenia-hid-winkey",
      "xenia-hid-xinput",
    })
