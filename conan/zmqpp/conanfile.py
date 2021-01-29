from conans import ConanFile, CMake, tools
from conans import AutoToolsBuildEnvironment

class ZmqppConan(ConanFile):
    name = "zmqpp"
    version = "4.2.0"
    license = "MPLv2"
    author = "Evgeny Chugunnyy (c00558140) chugunnyy.evgeny.alekseevich@huawei.com"
    url = "https://github.com/zeromq/zmqpp"
    description = "This C++ binding for 0mq/zmq is a 'high-level' library that hides most of the c-style interface core 0mq provides."
    topics = ("zmq", "0mq", "ZeroMQ")
    settings = "os", "compiler", "build_type", "arch"
    requires = "zeromq/4.3.3"
    options = {"shared": [True, False]}
    default_options = {"shared": False}
    generators = "make"

    def source(self):
        self.run("git clone https://github.com/zeromq/zmqpp.git")
        # zmpqq Makefile uses standard compiler variable name, usefull for adding extra libs, so we rename it.
        tools.replace_in_file("zmqpp/Makefile", "LIBRARY_PATH = $(SRC_PATH)/$(LIBRARY_DIR)",
                              "LIBRARY_PATH1 = $(SRC_PATH)/$(LIBRARY_DIR)\n$(info ENV==$(shell env))")
        tools.replace_in_file("zmqpp/Makefile", "ALL_LIBRARY_OBJECTS := $(patsubst $(SRC_PATH)/%.cpp, $(OBJECT_PATH)/%.o, $(shell find $(LIBRARY_PATH) -iname '*.cpp'))",
                              "ALL_LIBRARY_OBJECTS := $(patsubst $(SRC_PATH)/%.cpp, $(OBJECT_PATH)/%.o, $(shell find $(LIBRARY_PATH1) -iname '*.cpp'))")
        tools.replace_in_file("zmqpp/Makefile", "ALL_LIBRARY_INCLUDES := $(shell find $(LIBRARY_PATH) -iname '*.hpp')",
                              "ALL_LIBRARY_INCLUDES := $(shell find $(LIBRARY_PATH1) -iname '*.hpp')")

    def build(self):
        with tools.chdir("zmqpp"):
            atools = AutoToolsBuildEnvironment(self)
            dir(atools)
            for property, value in vars(atools).items():
                print(property, ":", value)
            incpath=':'.join(atools.include_paths)
            libpath=':'.join(atools.library_paths)
            buildVars = atools.vars #add dependencies paths for building
            buildVars["CPLUS_INCLUDE_PATH"]=incpath
            buildVars["LIBRARY_PATH"]=libpath
            print("buildVars=")
            print(buildVars)
            atools.make(vars=buildVars)

    def package(self):
        self.copy("*.hpp", dst="include/zmqpp", src="zmqpp/src/zmqpp")
        self.copy("*hello.lib", dst="lib", keep_path=False)
        self.copy("*.dll", dst="bin", keep_path=False)
        self.copy("*.so", dst="lib", keep_path=False)
        self.copy("*.dylib", dst="lib", keep_path=False)
        self.copy("*.a", dst="lib", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ["zmqpp"]
