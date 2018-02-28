import os
import sys
import re
import commands

def hWin():
	return "win" in sys.platform.lower()

def hSol():
	return "sunos" in sys.platform.lower()

def hLin():
	return "linux" in sys.platform.lower()

def hHpux():
	return "hp-ux11" in sys.platform.lower()

def hAix():
	return "aix6" in sys.platform.lower()

def split(val):
	result = []
	if val:
		for line in str(val).splitlines():
			sline = line.strip()
			cpos = sline.find("#")
			if cpos == -1:
				result.extend(sline.split())
			elif cpos:
				result.extend(sline[:cpos].split())
	return result

def makeList(obj):
	if not obj:
		return []
	if isinstance(obj, basestring):
		return split(obj)
	assert hasattr(obj, "__iter__")
	return filter(None, obj)

def checkPrefixAndSufix(path, pre, suf):
	prefix = ''
	sufix = ''
	indir, bname = os.path.split(path)
	if pre and not bname.startswith(pre):
		prefix = pre
	if suf and not bname.endswith(suf):
		sufix = suf
	return os.path.join(indir, "%s%s%s" % (prefix, bname, sufix))

def toProgName(path):
	# checkPrefixAndSufix(path, "", ".exe")
	return path

def dlibExt():
	if hWin():
		return ".dll"
	return ".so"

def slibExt():
	if hWin():
		return ".lib"
	return ".a"

def toStatName(path):
	prefix = "lib"
	if hWin():
		prefix = ""
	return checkPrefixAndSufix(path, prefix, slibExt())

def toDynName(path):
	return checkPrefixAndSufix(path, "lib", dlibExt())

def showConf(confStr, env):
	print (" = = = ")
	print ("\tCONFIG     := %s" % confStr)
	print ("\tCXX        := %s" % env.subst("$CXX"))
	print ("\tFLAGS      := %s" % env.subst("$CXXFLAGS $CCFLAGS $CPPFLAGS"))
	# usually env['_CCCOMCOM'] = '$CPPFLAGS $_CPPDEFFLAGS $_CPPINCFLAGS'
	#print ("\tCPPFLAGS   := %s" % env.subst("$CPPFLAGS"))
	#print ("\tCXXFLAGS   := %s" % env.subst("$CXXFLAGS"))
	#print ("\tSHCCFLAGS  := %s" % env.subst("$SHCCFLAGS"))
	print ("\tCPPDEFINES := %s" % env.subst("$_CPPDEFFLAGS"))
	print ("\tCPPPATH    := %s" % env.subst("$_CPPINCFLAGS"))
	print ("\tLINKFLAGS  := %s" % env.subst("$LINKFLAGS"))
	print ("\tLIBPATH    := %s" % env.subst("$_LIBDIRFLAGS"))
	print ("\tLIBS       := %s" % env.subst("$_LIBFLAGS"))
	print ("\tCXXCOM     := %s" % env.get("CXXCOM"))
	print (" = = = ")

#
#	registering function to be accessible inside SConscripts
#
def registerFunction(name, func):
	import SCons.Script
	if hasattr(SCons.Script, name):
		raise RuntimeError("Duplicated attribute: %s already exists!" % name)
	setattr(SCons.Script, name, func)

def exportedFunction(origFunc):
	registerFunction(origFunc.__name__, origFunc)
	def func(*args, **kwargs):
		return origFunc(*args, **kwargs)
	return func

def exportedFunctionAs(*args):
	def calledWithOrig(origFunc):
		for arg in args:
			registerFunction(arg, origFunc)
	return calledWithOrig

class Ver:
	def __init__(self, ver):
		toInt = lambda x: int("0"+re.compile("(\d*)").search(x).group(0))
		self.__ints = map(toInt, str(ver).split('.'))

	def __cmp__(self, other):
		# pylint: disable=W0212
		if not isinstance(other, Ver):
			return -1
		for va, vb in zip(self.__ints, other.__ints):
			res = cmp(va, vb)
			if res:
				return res
		return cmp(len(self.__ints), len(other.__ints))

def isVersionLessThan(versionA, versionB):
	"""
	>>> isVersionLessThan("", "4.0")
	True
	>>> isVersionLessThan("1.1", "4.0")
	True
	>>> isVersionLessThan("error", "4.0")
	True
	>>> isVersionLessThan("2.0", "4.0")
	True
	>>> isVersionLessThan("4.0", "4.0")
	False
	>>> isVersionLessThan("4.1.3.whatever.34", "4.0")
	False
	"""
	return Ver(versionA) < Ver(versionB)

def getCXXVersion(env):
	return commands.getoutput(env["CXX"] + " -dumpversion")

