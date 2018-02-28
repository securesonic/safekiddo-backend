import sys

MARKER_NAME = "__config_flag_marker__"

def markModifier(origFunc):
	"""
		decorator for marking function as defined for supporting flag as named
	"""
	def wrapper(*args, **kwargs):
		return origFunc(*args, **kwargs)
	setattr(wrapper, MARKER_NAME, origFunc.__name__)
	return wrapper

def markModifierAs(configFlag):
	"""
		decorator for marking function as defined for supporting "configFlag"
	"""
	def marker(origFunc):
		def wrapper(*args, **kwargs):
			return origFunc(*args, **kwargs)
		setattr(wrapper, MARKER_NAME, configFlag)
		return wrapper
	return marker

class ProjectConfig(object):
	bases = set()
	flags = set()
	numeric = set()

	def __init__(self, configFlags):
		self.configFlags = configFlags
		if self.__class__.__name__ == "ProjectConfig":
			raise RuntimeError("InternalError:: Please create object of any subclass of ProjectConfig...")

	def __filterForBaseFlag(self, useFuncs, baseFlag, exeFuncs):
		"""
			select functions to be called due to 'baseFlag'
		"""
		ok = False
		for (aName, aVal) in sorted(useFuncs.iteritems()):
			if aName.startswith("no_"):
				bFunc = aName[3:]
				if bFunc != baseFlag:
					exeFuncs.append(aVal)
			elif aName == baseFlag:
				exeFuncs.insert(0, aVal)
				ok = True
		if not ok:
			self._raiseError(invalid = baseFlag, isbase = True)

	def __filterForOtherFlags(self, useFlagFuncs, useNoFlagFuncs, flags, exeFuncs):
		disableNoFuncs = set()
		for flag in flags:
			fVal = useFlagFuncs.get(flag, None)
			if fVal:
				if fVal.__name__.endswith("_INT"):
					self._raiseError(invalid = "%s (int value required)" % flag)
				disableNoFuncs.add(flag)
			else:
				for candint in self.numeric:
					if not flag.startswith(candint):
						continue
					funcInt = useFlagFuncs.get(candint, None)
					try:
						intval = int(flag[len(candint):])
					except ValueError:
						continue
					fVal = lambda x: funcInt(self, intval)
					fVal.__name__ = "<lambda>: %s(%s)" % (funcInt.__name__, intval)
					disableNoFuncs.add(candint)

			if not fVal:
				if flag in self.flags: # flag already handled
					continue
				self._raiseError(invalid = flag)
			exeFuncs.append(fVal)
		for fName, fVal in sorted(useNoFlagFuncs.iteritems()):
			if fName not in disableNoFuncs:
				exeFuncs.append(fVal)

	# support for flags defined in self.configFlags
	def configure(self):
		useBaseFuncs = {}
		useFlagFuncs = {}
		useNoFlagFuncs = {}
#		for fVal in self.__class__.__dict__.itervalues():
#			fName = getattr(fVal, MARKER_NAME, None)
		for fName in dir(self):
			fVal = getattr(self, fName)
			if fName.startswith("_") or not callable(fVal):
				continue
			#print fVal, fName
			if fName:
				if fName.startswith("BASE_"):
					useBaseFuncs[fName[5:]] = fVal
				else:
					if fName.startswith("no_") or fName.startswith("FLAG_no_"):
						aName = fName.split("no_", 1)[-1]
						if aName.endswith("_INT"):
							aName = aName[:-4]
						useNoFlagFuncs[aName] = fVal
					else:
						aName = fName
						if fName.startswith("FLAG_"):
							aName = fName[5:]
						if aName.endswith("_INT"):
							aName = aName[:-4]
							self.numeric.add(aName)
						useFlagFuncs[aName] = fVal

		if not self.configFlags:
			raise RuntimeError("empty CONFIG(!?)")
		base, otherFlags = self.configFlags[0], self.configFlags[1:]
		#
		exeFuncs = []
		self.__filterForBaseFlag(useBaseFuncs, base, exeFuncs)
		self.__filterForOtherFlags(useFlagFuncs, useNoFlagFuncs, otherFlags, exeFuncs)
		for fVal in exeFuncs:
			#print "call: %s" % fVal.__name__
			fVal()

	def _raiseError(self, invalid = None, isbase = False):
		bases = set()
		flags = set()
		flags.update(self.flags)
		for attrName in dir(self):
			aVal = getattr(self, attrName)
			if not callable(aVal) or attrName.startswith("_"):
				continue
			toSet = flags
			flagMod = attrName
			if flagMod.startswith("BASE_"):
				flagMod = flagMod[5:]
				toSet = bases
			elif flagMod.startswith("FLAG_"):
				flagMod = flagMod[5:]
			if flagMod.startswith("no_"):
				flagMod = flagMod[3:]
			if "_" in flagMod:
				continue
			toSet.add(flagMod)
		for flag in self.numeric:
			flags.add("%s<number>" % flag)
			flags.discard("%s_INT" % flag)

		sys.stderr.write("Used instance definitions: %s\n" % self.__class__.__name__)
		sys.stderr.write("CONFIG contain invalid %sflag: %s\n" % ("base "*bool(isbase), invalid))
		sys.stderr.write("Valid base flags:\n")
		sys.stderr.write("\t %s\n\n" % ", ".join(sorted(bases)))
		sys.stderr.write("Valid other flags:\n")
		sys.stderr.write("\t %s\n\n" % ", ".join(sorted(flags)))
		sys.exit(1)

	def is32Bit(self): # pylint: disable=C0103
		return "32" in self.configFlags

	def is64Bit(self): # pylint: disable=C0103
		return "32" not in self.configFlags

	def isDebug(self):
		return "debug" in self.configFlags

	def isRelease(self):
		return "release" not in self.configFlags

	def getString(self):
		return "_".join(self.configFlags)

class BitVerifyConfig(ProjectConfig):
	pass

def is32Bit(buildConfig):
	return BitVerifyConfig(str(buildConfig).split("_")).is32Bit()

def is64Bit(buildConfig):
	return BitVerifyConfig(str(buildConfig).split("_")).is64Bit()

def isDebug(buildConfig):
	return BitVerifyConfig(str(buildConfig).split("_")).isDebug()

def isRelease(buildConfig):
	return BitVerifyConfig(str(buildConfig).split("_")).isRelease()
