import os
import glob

import loglib
import wrplib
import protocTool
import SCons.Environment

class BuildManager:
	def __init__(self):
		self.origdir = os.getcwd()
		# warning: for origdir = '.' scons may loose library dependency (!)
		#self.origdir = "."
		self.env = SCons.Environment.Environment()
		protocTool.addTool(self.env)
		self.cdir = "."
		self.objdir = "obj"
		self.bindir = "bin"
		self.unittests = []
		self.modules = []
		self.all = []
		self.uni = { }
		self.alias = None
		self.__libs = {}
		for param in  [ 'PATH', 'QTDIR', 'QTINC', 'QTLIB', 'OSTYPE', 'LOGNAME', 'USER', 'UID', 'TERM', 'HOME',
					'DISTCC_HOSTS', 'CCACHE_PREFIX', 'CCACHE_DIR',
					'HS_BUILD_MASTER', 'HS_LOGS_COUNT', 'HOSTNAME', 'HOSTTYPE', 'BASH', 'BASH_VERSION' ]:
			value = os.getenv(param, None)
			if value is not None:
				self.env["ENV"][param] = value

	def setLibAlias(self, name, libs, checkOverride=True):
		"""
			Aliasses for libraries. See doctest in expandLibAliasses for examples.
		"""
		prev = self.__libs.get(name, None)
		assert not checkOverride or prev is None, "Library alias %s already defined: %s!" % (name, prev)
		libs = wrplib.makeList(libs)
		loglib.debug("LIB ALIAS: %s => %s" % (name, ", ".join(libs)))
		self.__libs[name] = libs

	def __libAliasExpander(self, value, done, result):
		if value in done:
			return
		done.add(value)
		isAlias = value in self.__libs
		if not isAlias:
			if value in result:
				return
			result.append(value)
			return
		for newLib in self.__libs[value]:
			if newLib == value:
				pass
			elif newLib in self.__libs:
				# newLib is alias then rather try to get expanded value
				self.__libAliasExpander(newLib, done, result)
				continue
			elif newLib in done:
				continue
			if newLib not in result:
				result.append(newLib)


	def expandLibAliasses(self, default, libs):
		"""
			>>> b=BuildManager()
			>>> b.setLibAlias('libA', ['libA', 'libB'])
			>>> b.expandLibAliasses([], 'libA')
			['libA', 'libB']
			>>> b.setLibAlias('libB', ['libD', 'libA', 'libC'])
			>>> b.expandLibAliasses('libB', 'libC')
			['libD', 'libA', 'libC']
			>>> b.setLibAlias('libAll', ['libA', 'libB', 'libC', 'libD'])
			>>> b.setLibAlias('libC', ['expandC'])
			>>> b.setLibAlias('libD', ['libD', 'expandD'])
			>>> b.expandLibAliasses('libF', 'libAll')
			['libF', 'libA', 'libD', 'expandD', 'expandC']
			>>> # ================ WARNING:
			>>> b.setLibAlias('libA', ['libB'], checkOverride=False)
			>>> b.setLibAlias('libB', ['libA'], checkOverride=False)
			>>> b.expandLibAliasses('libA', 'libB')
			[]
			>>> b.expandLibAliasses('libB', 'libA')
			[]
		"""
		done = set()
		result = []
		for lib in wrplib.makeList(default):
			self.__libAliasExpander(lib, done, result)
		for lib in wrplib.makeList(libs):
			self.__libAliasExpander(lib, done, result)
		return result

	def build(self, subdirectory, toAlias = None):
		#pathElts = subdir.split(os.sep)
		#isTest = ("unittest" in pathElts) or ("unittests" in pathElts) 
		scpath = os.path.join(subdirectory, "SConscript")
		if os.path.exists(scpath):
			olddir = self.cdir
			oldenv = self.env
			oldalias = self.alias
			self.cdir = os.path.join(self.cdir, subdirectory)
			if toAlias is not None:
				self.alias = toAlias
			try:
				self.env = oldenv.Clone()
				self.env.SConscript(scpath) # pylint: disable=E1103
			finally:
				self.env = oldenv
				self.cdir = olddir
				self.alias = oldalias
		else:
			raise RuntimeError("Invalid subdirectory: %s - there is no SConscript" % subdirectory)

	def toObjPath(self, name):
		pathWithBase, _ext = os.path.splitext(name)
		return os.path.join(self.objdir, pathWithBase)

	def toTarget(self, name):
		return os.path.join(self.origdir, self.bindir, name)

	def register(self, objs, isTest, dest):
		self.all.append(objs)
		if isTest:
			suffix = "_unittests"
		else:
			suffix = "_modules"
		toKeys = []
		if self.alias:
			toKeys.append(self.alias)
			toKeys.append(self.alias + suffix)
		if dest:
			toKeys.append(dest)
			toKeys.append(dest + suffix)
		for key in toKeys:
			has = self.uni.get(key, None)
			if has is None:
				self.uni[key] = [ objs ]
			else:
				has.append(objs)

	def finalize(self):
		infos = ["all"]
		self.env.Alias("all", self.all)
		for (key, vals) in self.uni.iteritems():
			self.env.Alias(key, vals)
			infos.append(key)
		infos.sort()
		loglib.debug("Defined aliases: %s", ", ".join(infos))
		loglib.debug("Targets count: %s" % len(self.all))

manager = BuildManager()

def resetEnv(): # used for tests - do not use after creation of hydraOSTConfig
	global manager
	manager = BuildManager()

def setLibAlias(name, libs, checkOverride=True):
	manager.setLibAlias(name, libs, checkOverride=checkOverride)

def __verifyDiscardVals(env, args):
	if env is None:
		env = manager.env
	if not args:
		raise RuntimeError("__verifyDiscardVals missed args to remove from [%s]" % env)
	return env, args

def discardFlagsFrom(env, name, *args):
	env, args = __verifyDiscardVals(env, args)
	for arg in args:
		try:
			env[name].remove(arg)
		except ValueError:
			pass

def discardCppDefines(env, *args):
	env, args = __verifyDiscardVals(env, args)
	for arg in args:
		try:
			env["CPPDEFINES"].pop(arg)
		except KeyError:
			pass

def setObjectDir(objdir):
	manager.objdir = objdir

def setTargetDir(bindir):
	manager.bindir = bindir

def getTargetDir():
	return manager.bindir

def subdir(scpath, alias = None):
	return manager.build(scpath, toAlias = alias)

def toAbsPath(path):
	return os.path.abspath(os.path.join(manager.origdir, path))

def toObjPath(path):
	return manager.toObjPath(path)

def toTarget(name):
	return manager.toTarget(name)

def getEnv():
	return manager.env

def getSources(patt):
	alls = glob.glob(patt)
	alls.sort()
	return alls

def __prepareCppSources(srcs):
	if srcs is None:
		srcs = getSources("*.cpp")
	else:
		if isinstance(srcs, basestring):
			srcs = [ srcs ]
		newSources = []
		for src in srcs:
			if isinstance(src, basestring):
				if "*" in src:
					newSources.extend(getSources(src))
				else:
					newSources.append(src)
			else:
				newSources.append(src)
		srcs = newSources
	return srcs

def getSrcFile(src):
	if src.startswith(os.sep):
		if src.startswith(manager.origdir):
			src = src[len(manager.origdir):].lstrip(os.sep)
		else:
			# ooops...
			raise RuntimeError("Can not resolve: absolute path in source is outside repository! [%s]" % src)
	else:
		src = os.path.join(manager.cdir, src)
	src = os.path.normpath(src)
	return src

def toObjBasePath(name, src):
	return manager.toObjPath(os.path.join(name, src))

def __srcsToObjs(sources, name, convFunc):
	res = []
	for src in sources:
		if isinstance(src, basestring):
			#print src, manager.cdir, os.getcwd()
			src = getSrcFile(src)
			target = toObjBasePath(name, src) + ".o"
			src = "#" + src
			target = "#" + target
			# print src, " => ", target
			res.append( convFunc(source = src, target = target) )
		else:
			res.append(src)
	return res

def register(name, objs, dest = None):
	# impotant: in sconscripts we often write "test/testName" instead of os.path.join("test", "testname"), for simplicity
	# that's why we have to handle both separators here
	isTest = name.startswith("tests/") or name.startswith("tests\\")
	manager.register(objs, isTest, dest)

def toStaticObjects(srcs, name, env):
	srcs = __prepareCppSources(srcs)
	objs = __srcsToObjs(srcs, name, env.StaticObject)
	return objs

def toSharedObjects(srcs, name, env):
	srcs = __prepareCppSources(srcs)
	objs = __srcsToObjs(srcs, name, env.SharedObject)
	return objs

def findCompiledObjects(name, srcs, env):
	targets = []
	for src in srcs:
		targets.append(manager.toObjPath(os.path.join(name, src)) + ".o")
	targets = map(toAbsPath, targets)
	return env.arg2nodes(targets)

def prepareEnv(env, libs):
	if env is None:
		env = getEnv()
	env = env.Clone()
	env["LIBS"] = manager.expandLibAliasses(env["LIBS"], libs)
	return env
