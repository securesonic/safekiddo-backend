## pylint: disable=C0103,W0511

import os
import sys

import projectConfig
import wrpManager
import wrplib
import loglib
import atexit
import reduceCacheSize


class SafeKiddoConfig(projectConfig.ProjectConfig):
	def __init__(self, configFlags):
		projectConfig.ProjectConfig.__init__(self, configFlags)
		loglib.info("Using configuration : %s" % self.__class__.__name__)
		# NOTE: self.env should be always base environment, any SConscript should not access it
		self.env = wrpManager.getEnv()
		self._setupSconsDefaults()
		self.__setupForAll()
		self._setupDefaultOpts()
		self._setupLibAliases()
		if "gcov" not in self.configFlags and int(os.getenv('ENABLE_SCONS_CACHE', 1)):
			# while compilation with gcov compilator save *.gcdo files that are needed to store
			# execution line counter. If binary is retrieved from cache then requested gcdo file
			# would be missed
			self._setupCache()
		
		self.configure() # this function executes method corresponding CONFIG flags

	def configure(self):
		projectConfig.ProjectConfig.configure(self)

	def _replaceVars(self, value):
		bits = "64"
		if self.is32Bit():
			bits = "32"
		config = self.getString()
		return value.replace("$(BITS)", bits).replace("$(CONFIG)", config)

	def _makeOpt(self, name):
		raise NotImplementedError

	def _setupSconsDefaults(self):
		raise NotImplementedError

	def _setupDefaultOpts(self):
		raise NotImplementedError

	def _setupDefaultBoostFilesystemVersion(self):
		self.env.AppendUnique(CPPDEFINES = {'BOOST_FILESYSTEM_VERSION' : "3" })

	def _getBoostRoot(self):
		raise NotImplementedError

	def _getBoostLibs(self, mt=False):
		raise NotImplementedError

	def _getPthreadCompilerFlag(self):
		raise NotImplementedError

	def _setupBoostPaths(self, path, version):
		self.env.AppendUnique(CPPPATH = [os.path.join(path, "include", version)])
		self.env.AppendUnique(LIBPATH = [os.path.join(path, self._replaceVars("lib$(BITS)"), version)])

	def _setupCache(self):
		destdir = reduceCacheSize.getCacheDir()
		loglib.info("CacheDir: %s", destdir)
		if not destdir.startswith('/remote/backup/'):
			atexit.register(lambda: reduceCacheSize.cacheCleanup(destdir))
		self.env.CacheDir(destdir)

	def _setupPthread(self, value, env):
		if value:
			pthreadFlag = self._getPthreadCompilerFlag()
			if not pthreadFlag:
				loglib.info("pthreadFlag=%s, skip setting up pthread enabled=%s" % (pthreadFlag, value))
				loglib.debug("pthreadFlag=%s, skip setting up pthread enabled=%s" % (pthreadFlag, value))
				return
			lib = "pthread"
			opt = self._makeOpt(pthreadFlag)
	
			env.AppendUnique(CCFLAGS=[opt])
			env.AppendUnique(LIBS=[lib])
			
	def _setPICFlag(self, env):
		pass

	def _setupLibAliases(self):
		boostLibsMT = self._getBoostLibs(mt=True)
		wrpManager.setLibAlias("boostLibs_ST", self._getBoostLibs())
		wrpManager.setLibAlias("boostLibs_MT", boostLibsMT)
		
	def __setupForAll(self):
		"""
			Place here the configuration common for all platforms.
		"""
		pass

	#@projectConfig.markModifier
	def BASE_release(self):
		raise NotImplementedError

	#@projectConfig.markModifier
	def BASE_debug(self):
		raise NotImplementedError

	#@projectConfig.markModifier
	def BASE_gcov(self):
		raise NotImplementedError

	#@projectConfig.markModifierAs("32")
	def FLAG_32(self):
		pass

	#@projectConfig.markModifierAs("prof")
	def FLAG_prof(self):
		self.env.AppendUnique(LIBS = ["profiler"])
		#self.env.AppendUnique(LIBPATH = ["/remote/beta38/hydra/zytka/workspaces/SafeKiddo/gperftools/gperftools-2.1/.libs"])

	#@projectConfig.markModifierAs("no_32")
	def FLAG_no_32(self):
		pass

	#@projectConfig.markModifierAs("64")
	def FLAG_64(self):
		pass

	#@projectConfig.markModifierAs("no_boost145")
	def FLAG_no_boost149(self):
		# FLAG_no_* means there is no flag, and in such case we use default boost version 
		self._setupBoostPaths(self._getBoostRoot(), "boost149")
		
	#@projectConfig.markModifierAs("boost145")
	def FLAG_boost149(self):
		self._setupBoostPaths(self._getBoostRoot(), "boost149")

	#@projectConfig.markModifierAs("prod")
	def prod(self):
		pass

# 	def getOpenSSLLibraryPath(self):
# 		return self._replaceVars(os.path.join(self._getOpenSSLLibRoot(), "lib$(BITS)"))

# 	#@projectConfig.markModifierAs("staticssl")
# 	def FLAG_staticssl(self):
# 		self.env.AppendUnique(CPPPATH = [os.path.join(self._getOpenSSLLibRoot(), "include")])
# 		self.env.AppendUnique(LIBPATH = [self.getOpenSSLLibraryPath()])
# 
# 		wrpManager.setLibAlias("sslST", "libssl")
# 		wrpManager.setLibAlias("sslMT", "libssl")
# 		wrpManager.setLibAlias("cryptoST", "libcrypto")
# 		wrpManager.setLibAlias("cryptoMT", "libcrypto")

# 	#@projectConfig.markModifierAs("no_staticssl")
# 	def FLAG_no_staticssl(self):
# 		wrpManager.setLibAlias("sslST", "ssl")
# 		wrpManager.setLibAlias("sslMT", "ssl")

	#@projectConfig.markModifier
	def opt(self):
		raise NotImplementedError

	#@projectConfig.markModifier
	def valgrind(self):
		raise NotImplementedError

	#@projectConfig.markModifier
	def sym(self):
		raise NotImplementedError

	#@projectConfig.markModifier
	def noOpt(self):
		raise NotImplementedError

	#@projectConfig.markModifierAs("exportsym")
	def FLAG_exportsym(self):
		#mark that plugin symbols should be exported for unit tests
		pass
	#@projectConfig.markModifierAs("no_exportsym")
	def FLAG_no_exportsym(self):
		pass

	def _getOstIncludeDir(self):
		raise NotImplementedError

	def _setupPluginEnv(self, env):
		return env

class SafeKiddoConfigUnix(SafeKiddoConfig):
	def _makeOpt(self, name):
		return "-%s" % name

	def _setupSconsDefaults(self):
		self.env["CXX"] = "g++"
		self.env["SHCXX"] = "g++"
		self.env["LIBS"] = []
		self.env["CXXFLAGS"] = []
		self.env["CCFLAGS"] = []
		self.env["CPPFLAGS"] = []
		self.env["LINKFLAGS"] = []

		self.env['ENV']['PATH'] = os.getenv("PATH") # forbid extending path by scons...
		
		# scons assume there is CC compiler which probably need -KPIC instead of -fPIC
		for compileFlags in (self.env["SHCXXFLAGS"], self.env["SHCCFLAGS"]):
			while "-KPIC" in compileFlags:
				compileFlags.remove("-KPIC")

	def _setupDefaultOpts(self):
		haveVersion = wrplib.getCXXVersion(self.env) 
		if not wrplib.isVersionLessThan(haveVersion, "4.0"):
			self.env.AppendUnique(CCFLAGS = [ "-Wall" ])
		if os.getenv("PERMISSIVE", None) != "1":
			self.env.AppendUnique(CCFLAGS = [ "-Werror" ])

                self.env.AppendUnique(CXXFLAGS = ['-std=gnu++11'])
		# lets adhere to strict aliasing rules for now...
		#self.env.AppendUnique(CCFLAGS = ["-fno-strict-aliasing"])	

		self.env.AppendUnique(CPPDEFINES = {
			"_FILE_OFFSET_BITS": "64"
		})
		self._setupDefaultBoostFilesystemVersion()

	def _getBoostRoot(self):
		return "/usr"

	def _getBoostLibs(self, mt=False):
		boostLibs = """
				boost_system
		""".split()
		if mt:
			boostLibs = map(lambda libName: libName + "-mt", boostLibs)
			boostLibs.append("boost_thread-mt")
		return boostLibs

	def _getPthreadCompilerFlag(self):
		return "pthread"

	def _setupLibAliases(self):
		SafeKiddoConfig._setupLibAliases(self)

	def BASE_release(self):
		#self.opt()
		self.env.AppendUnique(CPPDEFINES = {"NDEBUG": '1'})

	def BASE_gcov(self):
		self.env.AppendUnique(CPPFLAGS = ['-ftest-coverage', '-fprofile-arcs'])
		self.env.AppendUnique(LINKFLAGS = ['-ftest-coverage', '-fprofile-arcs'])
		self.env.AppendUnique(CPPDEFINES = {"GCOV_ENABLED": ''})

	def BASE_debug(self):
		self.env.AppendUnique(CCFLAGS = ["-g"])
		# we use COOL_DEBUG_LOG and COOL_ASSERT also without coolkit, just to have the same defines as in coolkit 
# 		self.env.AppendUnique(CPPDEFINES = {"CACHE_LOG_ON": ''}) # webroot bcap cache 

	def FLAG_32(self):
		self.env.AppendUnique(CCFLAGS = ["-m32"])
		self.env.AppendUnique(LINKFLAGS = ["-m32"])

	def FLAG_no_32(self):
		self.env.AppendUnique(CCFLAGS = ["-m64"])
		self.env.AppendUnique(LINKFLAGS = ["-m64"])

	def FLAG_pg93(self):
		self.env.AppendUnique(CPPPATH = ["/usr/pgsql-9.3/include"])
		self.env.AppendUnique(LIBPATH = ["/usr/pgsql-9.3/lib"])

	def valgrind(self):
		self.env.AppendUnique(CCFLAGS = ['-fno-tree-ccp', '-fno-tree-dominator-opts'])
		#https://svn.boost.org/trac/boost/ticket/4946 - unused variable in lexical_cast in valgrind
		self.env.AppendUnique(CCFLAGS = ['-Wno-uninitialized'])
		self.env.AppendUnique(LINKFLAGS = ['-fno-tree-ccp', '-fno-tree-dominator-opts'])
		

	def sym(self):
		wrpManager.discardFlagsFrom(self.env, 'CCFLAGS', '-g0')
		self.env.AppendUnique(CCFLAGS = ["-ggdb3"])

	def __clearOpt(self):
		env = self.env
		wrpManager.discardFlagsFrom(env, 'CCFLAGS', ['-O0', '-O1', '-O2', '-O3', '-fno-inline'])
		wrpManager.discardCppDefines(env, 'OPT_LEVEL')

	def opt(self):
		self.__clearOpt()
		self.env.AppendUnique(CCFLAGS = [ "-O3"])
		self.env.AppendUnique(CPPDEFINES = { "OPT_LEVEL": "3" } )

	def noOpt(self):
		self.__clearOpt()
		self.env.AppendUnique(CPPDEFINES = { "OPT_LEVEL": "0" } )
		self.env.AppendUnique(CCFLAGS = ["-O0", '-fno-inline'])

	def _setPICFlag(self, env):
		env.AppendUnique(CCFLAGS = ["-fPIC"])

class SafeKiddoConfigLin(SafeKiddoConfigUnix):
	def __init__(self, configFlags):
		# internal variables must be configured before calling superclass constructor because 
		# the superclass initialization uses some functions that are redefined here
		self.__boostPath = None

		SafeKiddoConfigUnix.__init__(self, configFlags)
		
	def _setupDefaultOpts(self):
		SafeKiddoConfigUnix._setupDefaultOpts(self)
		self.env.AppendUnique(LIBS = ["stdc++"])
		
	def BASE_no_gcov(self):
		# for some unspecified reason -export-dynamic caused broken gcov report
		# - add -export-dynamic ONLY if it's not a gcov config
		
		self.env.AppendUnique(LINKFLAGS = ['-export-dynamic'])

	def FLAG_boost149(self):
		self.__boostPath = SafeKiddoConfigUnix._getBoostRoot(self)
		self._setupBoostPaths(self.__boostPath, "boost149")

ConfigDefault = SafeKiddoConfigLin
