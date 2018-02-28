import os
import platform
import wrpManager

config = os.environ["CONFIG"]
hConfigure(config)

hSetObjectDirectory(os.path.join("obj", config))
hSetTargetDirectory(os.path.join("bin", config))
hSetupPthreadEnabled(True)

# just to show less information for stdout:
# hHideBuildCommands()

BOOST_LIBS = """
    date_time
    filesystem
    program_options
    regex
    system
    thread
""".split()

# FIXME: this depends on env vars, for example if _GLIBCXX_DEBUG is in CPPDEFINES then suffix should be -mt-d
if platform.platform().find('centos') != -1:
	boostSuffix = "-mt"
else:
	boostSuffix = ""
	
for lib in BOOST_LIBS:
	hSetGlobLibAlias("boost_" + lib, "boost_" + lib + boostSuffix)


env = hEnv()
hAddIncludePaths("#/src/include")
hAddIncludePaths("#/external/projects/webroot/inc")
hAddIncludePaths("#/external/include")

generatedFilesIncludeDir = '#' + wrpManager.toObjPath('generated/includes')
hAddIncludePaths(generatedFilesIncludeDir)
Export('generatedFilesIncludeDir')

rootPath = Dir('#').abspath
# This is convenient in the following case:
# * there are hDynamicLib's A and B
# * A depends on B
# * prog.exe depends on A
# Without this line, we have to add a dependency for prog.exe also on B. In runtime the dependency is found thanks to
# $ORIGIN in RPATH. But during building it doesn't work, therefore we add explicit path for link-time rpath.
env.AppendUnique(LINKFLAGS = ["-Wl,-rpath-link=" + rootPath + "/bin/" + config])

env['PROGSUFFIX'] = '.exe'

hShowConf()

if int(os.environ["BUILD_BACKEND"]) == 1:
	hSubdir("external")
	hSubdir("src/tests")

hSubdir("src/modules")

if int(os.environ["BUILD_ICAP_PLUGIN"]) == 1:
	hSubdir("src/icap/plugin")

hFinalize()
