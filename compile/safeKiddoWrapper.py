import os
import sys
import errno

import wrplib
import wrpManager
import safeKiddoConfig

exportedFunction = wrplib.exportedFunction
exportedFunctionAs = wrplib.exportedFunctionAs

# pylint: disable=W0105

"""
	--------------------------------
		global configuration
	--------------------------------
"""
#@exportedFunction
def hHideBuildCommands():
	env = wrpManager.getEnv()
	env['CCCOMSTR'] = 'cc: $TARGETS'
	env['SHCCCOMSTR'] = 'shcc: $TARGETS'
	env['CXXCOMSTR'] = 'cxx: $TARGETS'
	env['SHCXXCOMSTR'] = 'shcxx: $TARGETS'
	env['LINKCOMSTR'] = 'link: $TARGETS'
	env['SHLINKCOMSTR'] = 'shlink: $TARGETS'
	env['ARCOMSTR'] = 'ar: $TARGETS'
	env['RANLIBCOMSTR'] = 'ranlib: $TARGETS'
	env['GENSTR'] = '$GEN_NAME: $TARGETS'
	env['MOCSTR'] = 'moc $TARGET'
	env['MOCSTR'] = 'moc $TARGET'
	env['INSTALLSTR'] = 'Install "$SOURCES" as "$TARGETS"'

#@exportedFunctionAs("hSetObjectDirectory")
def hSetObjectDirectory(objdir):
	wrpManager.setObjectDir(objdir)

#@exportedFunction
def hSetTargetDirectory(bindir):
	wrpManager.setTargetDir(bindir)
	env = wrpManager.getEnv()
	abspath = os.path.abspath(bindir)
	env.Append(RPATH=[env.Literal('\\$$ORIGIN')])
	env.AppendUnique(LIBPATH=abspath)

"""
	--------------------------------
		environment configuration
	--------------------------------
"""
#@exportedFunction
def hEnv():
	return wrpManager.getEnv()

config = None

#@exportedFunction
def hConfigure(configStr):
	global config
	if not configStr:
		print >> sys.stderr, " >>> ---------------------------"
		print >> sys.stderr, " >>> $CONFIG may not be empty!!!"
		print >> sys.stderr, " >>> ---------------------------"
		sys.exit(1)
	configFlags = configStr.split("_")
# 	if hWin():
# 		config = hydraOSTConfig.HydraOSTConfigWin(configFlags)
# 	elif hSol():
# 		config = hydraOSTConfig.HydraOSTConfigSol(configFlags)
# 	elif hHpux():
# 		config = hydraOSTConfig.HydraOSTConfigHpux(configFlags)
# 	elif hLin():
# 		config = hydraOSTConfig.HydraOSTConfigLin(configFlags)
# 	elif hAix():
# 		config = hydraOSTConfig.HydraOSTConfigAix(configFlags)	
# 	else:
# 		print >> sys.stderr, "WARNING: unrecognized platform, using UNIX"
# 		config = hydraOSTConfig.HydraOSTConfigUnix(configFlags)
	config = safeKiddoConfig.ConfigDefault(configFlags)
	boostPath = config._getBoostRoot()
	if not os.path.exists(boostPath):
		raise RuntimeError("Does not exists: %s (missed boost directory)" % boostPath)
	return config

#@exportedFunction
def hAddFlags(*args, **envDef):
	env = envDef.get("env", None)
	if not env:
		env = wrpManager.getEnv()
	for arg in args:
		env.AppendUnique(CCFLAGS = arg)

#@exportedFunction
def hDiscardFlags(*args, **envDef):
	env = envDef.get('env', None)
	wrpManager.discardFlagsFrom(env, "CCFLAGS", *args)

#@exportedFunction
def hDiscardCppFlags(*args, **envDef):
	env = envDef.get('env', None)
	wrpManager.discardFlagsFrom(env, "CPPFLAGS", *args)

#@exportedFunction
def hAddLinkFlags(*args, **envDef):
	env = envDef.get('env', None)
	if not env:
		env = wrpManager.getEnv()
	for arg in args:
		env.AppendUnique(LINKFLAGS = arg)

#@exportedFunction
def hDiscardLinkFlags(*args, **envDef):
	env = envDef.get('env', None)
	wrpManager.discardFlagsFrom(env, "LINKFLAGS", *args)

#@exportedFunction
def hAddCppDefine(key, val = None, env = None):
	if env is None:
		env = wrpManager.getEnv()
	env.Append(CPPDEFINES = {key: val})

#@exportedFunction
def hDiscardCppDefine(*args, **envDef):
	wrpManager.discardCppDefines(envDef.get('env', None), *args)

#@exportedFunction
def hAddIncludePaths(*args, **envDef):
	env = envDef.get('env', None)
	if env is None:
		env = wrpManager.getEnv()
	env.AppendUnique(CPPPATH = map(None, args)) #map(None, args) => just convert tuple to list

#@exportedFunction
def hAddIncludePath(path, env = None):
	hAddIncludePaths(path, env = env)

#@exportedFunction
def hAddLibraryPath(path, env = None):
	if env is None:
		env = wrpManager.getEnv()
	env.AppendUnique(LIBPATH = [path])

def hSetupPthreadEnabled(value, env=None):
	if not env:
		env = hEnv()
	config._setupPthread(value, env=env)

def hSetPICFlag(env=None):
	if not env:
		env = hEnv()
	if config.is64Bit() and not hHpux():
		config._setPICFlag(env)	
		
		
#@exportedFunction
def hSetGlobLibAlias(name, libs, checkOverride=True):
	wrpManager.manager.setLibAlias(name, libs, checkOverride=checkOverride)

"""
	--------------------------------
		dependencies management
	--------------------------------
"""

def hInstallProject(projectName):
	"""
		This is lame implementation. We do not require our dependencies to know how to compile in our way on all our platforms,
		we just need access to their sources.
	"""
	print "*" * (len(projectName) + 15)
	print "* Installing " + projectName + " *"
	print "*" * (len(projectName) + 15)
	# create ext/projectName:
	try:
		os.makedirs(os.path.join("ext", projectName))
	except OSError, exc:
		if(exc.errno != errno.EEXIST):
			raise
	# create ext/projectName/src
	sourcePathForLink = os.path.join("..", "..", "..", projectName, "src")
	sourcePathForCopy = os.path.join("..", projectName, "src")
	targetPath = os.path.join("ext", projectName, "src")
	try:
		if os.name == 'posix':
			os.symlink(sourcePathForLink, targetPath)
		else:
			if os.path.exists(targetPath):
				os.remove(targetPath)

			import shutil #yes, we use python 2.7+ on windows
			shutil.copytree(sourcePathForCopy, targetPath, symlinks=True)
		print "Installation done."
	except OSError, exc:
		if(exc.errno == errno.EEXIST):
			print "Already installed."
		else:
			raise


"""
	 - intermediate - targets -
"""
#@exportedFunction
def hStaticObjs(name, srcs, env = None):
	if env is None:
		env = wrpManager.getEnv()
	return wrpManager.toStaticObjects(srcs, name, env)

#@exportedFunctionAs("hDynamicObjs", "hSharedObjs", "hDynamicObjects", "hSharedObjects")
def hDynamicObjs(name, srcs, env = None):
	if env is None:
		env = wrpManager.getEnv()
	return wrpManager.toSharedObjects(srcs, name, env)

#@exportedFunction
def hFindCompiled(name, srcs, env = None):
	if env is None:
		env = wrpManager.getEnv()
	return wrpManager.findCompiledObjects(name, srcs, env)

"""
	--------------------------------
		 - T A R G E T S -
	--------------------------------
"""
#@exportedFunctionAs("hDynamicLib", "hSharedLib")
def hDynamicLib(name, srcs = None, libs = None, env = None, alias = None, objectDir = None):
	if objectDir is None:
		objectDir = name

	isTest = name.startswith("tests")
	env = wrpManager.prepareEnv(env, libs)
	srcs = wrpManager.toSharedObjects(srcs, objectDir, env)
	targetPath = wrpManager.toTarget(name)
	targetPath = wrplib.toDynName(targetPath)
	if isTest:
		env.AppendUnique(RPATH = [env.Literal(os.path.join('\\$$ORIGIN', ".."))])
		env.AppendUnique(LIBPATH = [os.path.dirname(targetPath)])
	result = env.SharedLibrary(source = srcs, target = targetPath)
	wrpManager.register(name, result, alias)
	return result

#@exportedFunction
def hStaticLib(name, srcs = None, env = None, alias = None):
	env = wrpManager.prepareEnv(env, None)
	srcs = wrpManager.toStaticObjects(srcs, name, env)
	targetPath = wrpManager.toTarget(name)
	targetPath = wrplib.toStatName(targetPath)
	result = env.StaticLibrary(source = srcs, target = targetPath)
	wrpManager.register(name, result, alias)
	return result
	
#@exportedFunction
def hProg(name, srcs = None, libs = None, env = None, alias = None):
	env = wrpManager.prepareEnv(env, libs)
	srcs = wrpManager.toStaticObjects(srcs, name, env)
	targetPath = wrpManager.toTarget(name)
	targetPath = wrplib.toProgName(targetPath)
	if name.startswith("tests"):
		env.AppendUnique(RPATH = [env.Literal(os.path.join('\\$$ORIGIN', ".."))])
		env.AppendUnique(LIBPATH = [os.path.dirname(targetPath)])
	result = env.Program(source = srcs, target = targetPath)
	wrpManager.register(name, result, alias)
	return result

#wrpManager.register(name, result, alias)

"""
	--------------------------------
		other usefull functions
	--------------------------------
"""
#@exportedFunction
def hGetConfig():
	return config

#@exportedFunction
def hTarget(name):
	return wrpManager.toTarget(name)

#@exportedFunction
def hTargetDynamicLib(name):
	return wrplib.toDynName(hTarget(name))

#@exportedFunction
def hGlob(patt, excludes = None):
	alls = wrpManager.getSources(patt)
	if excludes:
		rmEntries = set()
		if isinstance(excludes, basestring):
			rmEntries.update(wrpManager.getSources(excludes))
		else:
			for exclude in excludes:
				rmEntries.update(wrpManager.getSources(exclude))
		alls = filter(lambda x: x not in rmEntries, alls)
	return alls

#@exportedFunction
def hSubdir(subdir, alias = None):
	return wrpManager.subdir(subdir, alias = alias)

#@exportedFunction
def hSubdirs(*args):
	for arg in args:
		for subdir in wrplib.split(arg):
			hSubdir(subdir)

#@exportedFunction
def hSplit(val):
	return wrplib.split(val)

#@exportedFunction
def hShowConf(env = None):
	if config is None:
		raise RuntimeError("Invalid use of 'hSetBoostRoot' - please use hConfigure first!")
	env = wrpManager.prepareEnv(env, None)
	wrplib.showConf(config.getString(), env)

#@exportedFunction
def hCppToExe(path):
	return os.path.basename(path).replace(".cpp", ".exe")

#@exportedFunction
def hFinalize():
	wrpManager.manager.finalize()

#@exportedFunction
def hWin():
	return wrplib.hWin()

#@exportedFunction
def hSol():
	return wrplib.hSol()

#@exportedFunction
def hLin():
	return wrplib.hLin()

#@exportedFunction
def hHpux():
	# hp-ux11
	return wrplib.hHpux()

#@exportedFunction
def hAix():
        # aix6
	return wrplib.hAix()

def hAddDLibExt(name):
	return str(name) + wrplib.dlibExt()

def hGetPluginEnv(env=None):
	if env is None:
		env = wrpManager.getEnv()
	config._setupPluginEnv(env)
	return env

def hGetCXXVersion(env):
	return wrplib.getCXXVersion(env)

def hDiscardFlagsFrom(env, name, *args):
	wrpManager.discardFlagsFrom(env, name, *args)

def hDiscardCppDefines(env, *args):
	wrpManager.discardCppDefines(env, *args)

def hGetVendorStype():
	return  config._getVendorStype()

def hAddSLibExt(name):
	return str(name) + wrplib.slibExt()

def hIsVersionLessThan(versionA, versionB):
	return wrplib.isVersionLessThan(versionA, versionB)

#TODO: https://svn.boost.org/trac/boost/ticket/4095
#but, g++ 4.1 does not support this option
# def hDisableArrayBoundsWarinig(env = None):
# 	if env is None:
# 		env = hEnv()
# 	if hSol() or hLin():
# 		if not hIsVersionLessThan(hGetCXXVersion(env), "4.3"):
# 			env.AppendUnique(CPPFLAGS = ["-Wno-array-bounds"])


def setupFunctions():
	globitems = globals().items()
	for functionName, functionCallback in globitems:
		if functionName.startswith("h") and type(functionCallback).__name__ == "function":
			wrplib.registerFunction(functionName, functionCallback)
